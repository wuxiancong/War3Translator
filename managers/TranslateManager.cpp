#include "../managers/TranslateManager.h"
#include "../managers/SettingsManager.h"
#include "../helpers/DebugHelper.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QEventLoop>
#include <QSslSocket>
#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QDir>

TranslateManager &TranslateManager::instance() {
    static TranslateManager instance;
    return instance;
}

TranslateManager::TranslateManager(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this))
{
    initTranslateManager();
}

TranslateManager::~TranslateManager() {
    cleanup();
}

void TranslateManager::cleanup()
{
    DebugHelper::recordTreeLog("chat_translate", "┌─ 🛑 TranslateManager 开始执行清理退出...", 0);

    if (m_apiRateTimer && m_apiRateTimer->isActive()) {
        m_apiRateTimer->stop();
        DebugHelper::recordTreeLog("chat_translate", "├─ ℹ️ 已停止 API 频率计时器", 1);
    }

    m_isProcessing = false;
    int remainingTasks = m_taskQueue.size();
    m_taskQueue.clear();
    DebugHelper::recordTreeLog("chat_translate", QString("├─ ℹ️ 已清空任务队列 (丢弃 %1 条未完成任务)").arg(remainingTasks), 1);

    if (!m_translationDb.isEmpty()) {
        DebugHelper::recordTreeLog("chat_translate", "├─ 💾 正在执行最后一次三端数据同步...", 1);
        saveCache();
    }

    DebugHelper::recordTreeLog("chat_translate", "└─ ✅ TranslateManager 清理完成。", 0, true);
}

void TranslateManager::initTranslateManager()
{
    bool supports = QSslSocket::supportsSsl();
    QString buildVersion = QSslSocket::sslLibraryBuildVersionString();
    QString runVersion = QSslSocket::sslLibraryVersionString();

    DebugHelper::recordTreeLog("chat_translate", "🔐 SSL 状态诊断:", 0);
    DebugHelper::recordTreeLog("chat_translate", QString("├─ 支持 SSL: %1").arg(supports ? "✅ 是" : "❌ 否"), 1);
    DebugHelper::recordTreeLog("chat_translate", QString("├─ 编译要求: %1").arg(buildVersion), 1);
    DebugHelper::recordTreeLog("chat_translate", QString("└─ 实际加载: %1").arg(runVersion.isEmpty() ? "未找到可用 OpenSSL DLL" : runVersion), 1, true);

    m_apiRateTimer = new QTimer(this);
    m_apiRateTimer->setSingleShot(true);
    connect(m_apiRateTimer, &QTimer::timeout, this, &TranslateManager::processNextTask);

    loadCache();
}

void TranslateManager::requestTranslationWithMetadata(quint32 pid, quint32 flag, quint32 extraScope, QString message, QString language)
{
    QElapsedTimer timer;
    timer.start();

    DebugHelper::recordTreeLog("chat_translate", "┌─ ⚙ [TranslateManager] 正在处理元数据翻译请求", 0);

    DebugHelper::recordTreeLog("chat_translate",
                               QString("├─ 👤 玩家上下文: PID=%1 | Flag=0x%2 | ExtraScope=0x%3")
                                   .arg(pid)
                                   .arg(QString::number(flag, 16).toUpper())
                                   .arg(QString::number(extraScope, 16).toUpper()), 1);

    DebugHelper::recordTreeLog("chat_translate",
                               QString("├─ 🌐 语种目标: %1 | 待翻译文本: \"%2\"").arg(language, message), 1);

    QString translatedMessage = translateSingleSync(message, language);

    if (!translatedMessage.isEmpty()) {
        DebugHelper::recordTreeLog("chat_translate",
                                   QString("├─ ✅ 转换完成: \"%1\"").arg(translatedMessage), 1);
    } else {
        DebugHelper::recordTreeLog("chat_translate", "├─ ❌ 转换失败: 未能从 API 获取到有效译文", 1);
    }

    DebugHelper::recordTreeLog("chat_translate",
                               QString("└─ 🏁 任务执行完毕 (API耗时: %1 ms)，正在发射 translationTaskFinished 信号").arg(timer.elapsed()), 0, true);

    emit translationTaskFinished(pid, flag, extraScope, message, translatedMessage);
}

void TranslateManager::requestAllTranslations(const QString &sourceText, const QString &sourceLangCode)
{
    if (sourceText.isEmpty()) return;

    if (SettingsManager::instance().isDefaultShoutContent(sourceText) ||
        SettingsManager::instance().isDefaultSchemeName(sourceText)) {
        DebugHelper::recordTreeLog("chat_translate",
                                   QString("ℹ️ 命中默认模板内容，跳过网络翻译: \"%1\"").arg(sourceText), 1);
        return;
    }

    if (m_translationDb.size() > 3000) {
        DebugHelper::recordTreeLog("chat_translate", "⚠ 触发自动容量维护...", 1);
        limitCacheSize(2000);
    }

    QString fromCode = "auto";
    DebugHelper::recordTreeLog("chat_translate", QString("┌─ 🌐 开启多语言翻译任务: \"%1\"").arg(sourceText), 0);
    QJsonObject item = m_translationDb.contains(sourceText) ? m_translationDb[sourceText].toObject() : QJsonObject();

    int addedTasks = 0;
    for (const QString &targetQtCode : SettingsManager::instance().languageCode()) {
        if (item.contains(targetQtCode)) {
            QString cachedVal = item.value(targetQtCode).toString();
            if (!cachedVal.isEmpty() && cachedVal != sourceText) continue;
        }

        m_taskQueue.enqueue({sourceText, fromCode, targetQtCode, 0});
        addedTasks++;
    }

    DebugHelper::recordTreeLog("chat_translate", QString("├─ 📋 待处理翻译语种数: %1").arg(addedTasks), 1);

    if (!m_isProcessing && addedTasks > 0) {
        m_isProcessing = true;
        emit isProcessingChanged();
        DebugHelper::recordTreeLog("chat_translate", "└─ 🚀 启动异步轮询器...", 1, true);
        processNextTask();
    } else {
        DebugHelper::recordTreeLog("chat_translate", addedTasks > 0 ? "└─ ⏳ 任务已排队" : "└─ ✅ 缓存完整，无需请求", 1, true);
    }
}

QString TranslateManager::translateSingleSync(const QString &sourceText, const QString &targetLangCode)
{
    if (sourceText.isEmpty()) return "";

    // 1. 优先检查本地缓存
    QString cached = getTranslation(sourceText, targetLangCode);
    if (!cached.isEmpty()) {
        return cached;
    }

    QString result = "";
    QElapsedTimer apiTimer;

    // --- 方案 A: Google 翻译 (首选) ---
    {
        apiTimer.start();
        QString gTo = mapToGoogleCode(targetLangCode);
        QUrl url(QString("https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=%1&dt=t&q=%2")
                     .arg(gTo, QUrl::toPercentEncoding(sourceText)));

        QNetworkRequest request(url);
        request.setTransferTimeout(m_translateTimeout);

        QNetworkReply *reply = m_networkManager->get(request);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isNull() && doc.isArray()) {
                QJsonArray mainArr = doc.array().at(0).toArray();
                for (int i = 0; i < mainArr.size(); ++i) {
                    result += mainArr.at(i).toArray().at(0).toString();
                }
            }
        }

        if (!result.isEmpty()) {
            reply->deleteLater();
            updateDbAndNotify(sourceText, targetLangCode, result);
             DebugHelper::recordTreeLog("chat_translate", QString("📡 Google 成功 (耗时: %1 ms)").arg(apiTimer.elapsed()), 2);
            return result;
        } else {
            handleGoogleError(reply);
            DebugHelper::recordTreeLog("chat_translate", QString("⚠ Google 失败 (耗时: %1 ms), 准备回退百度...").arg(apiTimer.elapsed()), 2);
        }
        reply->deleteLater();
    }

    // --- 方案 B: 百度翻译 (备选) ---
    {
        apiTimer.restart();
        QString bFrom = "auto";
        QString bTo = mapToBaiduCode(targetLangCode);
        QString salt = QString::number(QRandomGenerator::global()->generate());
        QString sign = QString(QCryptographicHash::hash(
                                   (m_appId + sourceText + salt + m_appKey).toUtf8(),
                                   QCryptographicHash::Md5).toHex());

        QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
        QUrlQuery query;
        query.addQueryItem("q", sourceText);
        query.addQueryItem("from", bFrom);
        query.addQueryItem("to", bTo);
        query.addQueryItem("appid", m_appId);
        query.addQueryItem("salt", salt);
        query.addQueryItem("sign", sign);
        url.setQuery(query);

        QNetworkRequest request(url);
        request.setTransferTimeout(m_translateTimeout);

        QNetworkReply *reply = m_networkManager->get(request);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
            if (doc.contains("error_code")) {
                handleBaiduTranslateError(doc["error_code"].toString(), bFrom, bTo);
            } else {
                QJsonArray results = doc["trans_result"].toArray();
                if (!results.isEmpty()) {
                    result = results.at(0).toObject()["dst"].toString();
                }
            }
        } else {
            DebugHelper::recordTreeLog("chat_translate", "❌ [Sync] 百度网络请求故障: " + reply->errorString(), 2);
        }

        if (!result.isEmpty()) {
            reply->deleteLater();
            updateDbAndNotify(sourceText, targetLangCode, result);
            DebugHelper::recordTreeLog("chat_translate", QString("│  📡 百度成功 (耗时: %1 ms)").arg(apiTimer.elapsed()), 2);
            return result;
        }
        reply->deleteLater();
    }

    return "";
}

void TranslateManager::processNextTask()
{
    // 1. 队列检查
    if (m_taskQueue.isEmpty()) {
        m_isProcessing = false;
        emit isProcessingChanged();
        DebugHelper::recordTreeLog("chat_translate", "🏁 所有排队翻译任务已完成，执行三端同步保存", 0);
        saveCache();
        return;
    }

    TranslationTask task = m_taskQueue.dequeue();
    QString qtTargetCode = task.toLang;

    // 2. 预校验：如果内存数据库已经有了这个翻译，直接跳过处理下一个
    if (m_translationDb.contains(task.originalText)) {
        QJsonObject translations = m_translationDb[task.originalText].toObject();
        if (translations.contains(qtTargetCode)) {
            QString currentVal = translations.value(qtTargetCode).toString();
            if (!currentVal.isEmpty() && currentVal != task.originalText) {
                QTimer::singleShot(10, this, &TranslateManager::processNextTask);
                return;
            }
        }
    }

    // --- 策略 A：Google 翻译 (retryCount == 0) ---
    if (task.retryCount == 0) {
        QString gFrom = (task.fromLang == "auto") ? "auto" : mapToGoogleCode(task.fromLang);
        QString gTo = mapToGoogleCode(qtTargetCode);

        QUrl url(QString("https://translate.googleapis.com/translate_a/single?client=gtx&sl=%1&tl=%2&dt=t&q=%3")
                     .arg(gFrom, gTo, QUrl::toPercentEncoding(task.originalText)));

        DebugHelper::recordTreeLog("chat_translate", QString("📡 [Google] 请求: %1 -> %2").arg(gFrom, gTo), 1);

        QNetworkRequest request(url);
        request.setTransferTimeout(3000);
        QNetworkReply *reply = m_networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [=]() {
            bool success = false;
            int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (reply->error() == QNetworkReply::NoError && httpCode == 200) {
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (!doc.isNull() && doc.isArray()) {
                    QJsonArray mainArr = doc.array().at(0).toArray();
                    QString dst = "";
                    for(int i = 0; i < mainArr.size(); ++i) {
                        dst += mainArr.at(i).toArray().at(0).toString();
                    }

                    if (!dst.isEmpty()) {
                        updateDbAndNotify(task.originalText, qtTargetCode, dst);
                        DebugHelper::recordTreeLog("chat_translate", "✅ [Google] 成功: " + dst, 2);
                        success = true;
                    }
                }
            }

            if (!success) {
                if (httpCode != 200 || reply->error() != QNetworkReply::NoError) {
                    handleGoogleError(reply);
                }

                DebugHelper::recordTreeLog("chat_translate", "⚠ [Google] 失败/超时，Fallback 至 [Baidu]", 2);

                TranslationTask fallbackTask = task;
                fallbackTask.retryCount = 1;
                m_taskQueue.prepend(fallbackTask);

                reply->deleteLater();
                m_apiRateTimer->start(50);
            } else {
                reply->deleteLater();
                m_apiRateTimer->start(500);
            }
        });
    }
    // --- 策略 B：Baidu 翻译 (retryCount == 1) ---
    else {
        QString bFrom = (task.fromLang == "auto") ? "auto" : mapToBaiduCode(task.fromLang);
        QString bTo   = mapToBaiduCode(qtTargetCode);

        QString salt = QString::number(QRandomGenerator::global()->generate());
        QString rawSign = m_appId + task.originalText + salt + m_appKey;
        QString sign = QString(QCryptographicHash::hash(rawSign.toUtf8(), QCryptographicHash::Md5).toHex());

        QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
        QUrlQuery query;
        query.addQueryItem("q", task.originalText);
        query.addQueryItem("from", bFrom);
        query.addQueryItem("to", bTo);
        query.addQueryItem("appid", m_appId);
        query.addQueryItem("salt", salt);
        query.addQueryItem("sign", sign);
        url.setQuery(query);

        DebugHelper::recordTreeLog("chat_translate", QString("📡 [Baidu] 请求: %1 -> %2").arg(bFrom, bTo), 1);

        QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonObject doc = QJsonDocument::fromJson(reply->readAll()).object();
                if (doc.contains("error_code")) {
                    QString errorCode = doc["error_code"].toString();
                    handleBaiduTranslateError(errorCode, bFrom, bTo);
                } else {
                    QJsonArray results = doc["trans_result"].toArray();
                    if (!results.isEmpty()) {
                        QString dst = results.at(0).toObject()["dst"].toString();
                        updateDbAndNotify(task.originalText, qtTargetCode, dst);
                        DebugHelper::recordTreeLog("chat_translate", QString("✅ [Baidu] 成功: %1").arg(dst), 2);
                    }
                }
            } else {
                DebugHelper::recordTreeLog("chat_translate", "⚠ [Baidu] 网络故障: " + reply->errorString(), 2);
            }
            reply->deleteLater();

            // 百度免费版限制 1s 1次，必须严格控制
            m_apiRateTimer->start(1100);
        });
    }
}

void TranslateManager::cancelTasksForText(const QString &text)
{
    if (text.isEmpty()) return;

    int initialSize = m_taskQueue.size();
    QQueue<TranslationTask> filteredQueue;
    while (!m_taskQueue.isEmpty()) {
        TranslationTask task = m_taskQueue.dequeue();
        if (task.originalText != text) {
            filteredQueue.enqueue(task);
        }
    }
    m_taskQueue = filteredQueue;

    if (m_translationDb.contains(text)) {
        m_translationDb.remove(text);
    }

    int removedCount = initialSize - m_taskQueue.size();
    if (removedCount > 0) {
        DebugHelper::recordTreeLog("chat_translate",
                                   QString("🗑 已取消旧任务：针对 \"%1\" 的 %2 个语种请求已撤回").arg(text).arg(removedCount), 1);
    }

    if (m_taskQueue.isEmpty() && m_isProcessing) {
        m_isProcessing = false;
        emit isProcessingChanged();
    }
}

void TranslateManager::updateDbAndNotify(const QString &originalText, const QString &qtLanguageCode, const QString &translatedText)
{
    QJsonObject item = m_translationDb[originalText].toObject();
    item.insert(qtLanguageCode, translatedText);
    m_translationDb.insert(originalText, item);
    emit cacheUpdated();
}

QString TranslateManager::getTranslation(const QString &sourceText, const QString &targetLangCode)
{
    if (m_translationDb.contains(sourceText)) {
        QJsonObject translations = m_translationDb[sourceText].toObject();
        if (translations.contains(targetLangCode)) {
            return translations.value(targetLangCode).toString();
        }
    }
    return "";
}

quint8 TranslateManager::getTranslationCount(const QString &sourceText)
{
    if (sourceText.isEmpty()) return 0;

    if (SettingsManager::instance().isDefaultShoutContent(sourceText) ||
        SettingsManager::instance().isDefaultSchemeName(sourceText)) {
        return 25;
    }

    QJsonObject targetTranslations;
    bool found = false;

    if (m_translationDb.contains(sourceText)) {
        targetTranslations = m_translationDb[sourceText].toObject();
        found = true;
    }
    else {
        for (auto it = m_translationDb.constBegin(); it != m_translationDb.constEnd(); ++it) {
            QJsonObject translations = it.value().toObject();
            for (auto langIt = translations.constBegin(); langIt != translations.constEnd(); ++langIt) {
                if (langIt.value().toString() == sourceText) {
                    targetTranslations = translations;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    if (found) {
        int count = 0;

        for (auto it = targetTranslations.constBegin(); it != targetTranslations.constEnd(); ++it) {
            QString val = it.value().toString();
            if (!val.isEmpty()) {
                count++;
            }
        }
        return static_cast<quint8>(count);
    }

    return 0;
}

void TranslateManager::loadCache()
{
    DebugHelper::recordTreeLog("chat_translate", "┌─ 🔍 开始三端加载翻译缓存 [优先级: 根目录 > 注册表 > AppData]", 0);

    // 1. 尝试从程序根目录加载 (优先级最高)
    QString rootPath = SettingsManager::instance().getConfigFilePath("i18n_cache.json");
    if (loadFromFile(rootPath, "根目录")) {
        DebugHelper::recordTreeLog("chat_translate", "└─ ✅ 从根目录加载成功", 1, true);
        return;
    }

    // 2. 尝试从注册表加载
    if (loadFromRegistry()) {
        DebugHelper::recordTreeLog("chat_translate", "└─ ✅ 从注册表加载成功", 1, true);
        return;
    }

    // 3. 尝试从 AppData 加载
    QString appDataPath = SettingsManager::instance().getAppDataConfigPath("i18n_cache.json");
    if (loadFromFile(appDataPath, "AppData")) {
        DebugHelper::recordTreeLog("chat_translate", "└─ ✅ 从AppData加载成功", 1, true);
        return;
    }

    if (!m_translationDb.isEmpty()) {
        emit cacheUpdated();
    }

    DebugHelper::recordTreeLog("chat_translate", "└─ ℹ️ 三端均无缓存数据，初始化新数据库", 1, true);
}

void TranslateManager::saveCache()
{
    if (m_translationDb.isEmpty()) return;

    DebugHelper::recordTreeLog("chat_translate", "┌─ 💾 启动三端同步持久化...", 0);

    QByteArray jsonData = QJsonDocument(m_translationDb).toJson(QJsonDocument::Compact);

    // 1. 保存到根目录
    QString rootPath = SettingsManager::instance().getConfigFilePath("i18n_cache.json");
    saveToFile(rootPath, jsonData, "根目录");

    // 2. 保存到注册表
    saveToRegistry(QString::fromUtf8(jsonData));

    // 3. 保存到 AppData
    QString appDataPath = SettingsManager::instance().getAppDataConfigPath("i18n_cache.json");
    saveToFile(appDataPath, jsonData, "AppData");

    DebugHelper::recordTreeLog("chat_translate", "└─ ✅ 三端同步任务执行完毕", 0, true);
}

void TranslateManager::limitCacheSize(int maxEntries)
{
    if (m_translationDb.size() <= maxEntries) return;

    DebugHelper::recordTreeLog("chat_translate", "⚠ 数据库超出容量限制，执行强制缩减...", 1);

    QStringList allKeys = m_translationDb.keys();
    int toRemove = m_translationDb.size() - maxEntries;
    for (int i = 0; i < toRemove; ++i) {
        m_translationDb.remove(allKeys.at(i));
    }
}

bool TranslateManager::loadFromFile(const QString &path, const QString &label) {
    QFile file(path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            m_translationDb = doc.object();
            file.close();
            DebugHelper::recordTreeLog("chat_translate", QString("├─ 📄 [%1] 读取成功 (%2 字节)").arg(label).arg(data.size()), 1);
            return true;
        }
        file.close();
    }
    return false;
}

bool TranslateManager::loadFromRegistry() {
    QString regPath = SettingsManager::instance().getRegistryPath("I18nCache");
    QSettings settings(regPath, QSettings::NativeFormat);
    QString jsonStr = settings.value("CacheData").toString();

    if (!jsonStr.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
        if (!doc.isNull()) {
            m_translationDb = doc.object();
            DebugHelper::recordTreeLog("chat_translate", QString("├─ 🔑 [注册表] 读取成功 (%1 字符)").arg(jsonStr.length()), 1);
            return true;
        }
    }
    return false;
}

void TranslateManager::saveToFile(const QString &path, const QByteArray &data, const QString &label) {
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(data);
        file.close();
        DebugHelper::recordTreeLog("chat_translate", QString("├─ 💾 [%1] 写入完成: %2").arg(label, path), 1);
    } else {
        DebugHelper::recordTreeLog("chat_translate", QString("├─ ⚠ [%1] 写入失败 (权限受限): %2").arg(label, path), 1);
    }
}

void TranslateManager::saveToRegistry(const QString &jsonStr) {
    try {
        QString regPath = SettingsManager::instance().getRegistryPath("I18nCache");
        QSettings settings(regPath, QSettings::NativeFormat);
        settings.setValue("CacheData", jsonStr);
        settings.sync();
        DebugHelper::recordTreeLog("chat_translate", "├─ 🔑 [注册表] 写入完成", 1);
    } catch (...) {
        DebugHelper::recordTreeLog("chat_translate", "├─ ❌ [注册表] 写入异常", 1);
    }
}

void TranslateManager::handleGoogleError(QNetworkReply *reply)
{
    int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QNetworkReply::NetworkError networkError = reply->error();

    QString diagnostic = "";

    if (networkError == QNetworkReply::OperationCanceledError || networkError == QNetworkReply::TimeoutError) {
        diagnostic = "请求超时。原因：在中国大陆可能需要开启代理才能访问 Google 翻译。";
    }
    else if (networkError == QNetworkReply::HostNotFoundError) {
        diagnostic = "无法解析 Google 主机。请检查网络连接。";
    }
    else {
        switch (httpCode) {
        case 429:
            diagnostic = "访问频率受限 (Too Many Requests)。原因：短时间内翻译请求过多，IP 已被 Google 临时封禁。建议：等待 1 小时后再试。";
            break;
        case 403:
            diagnostic = "拒绝访问 (Forbidden)。原因：可能是 User-Agent 被封禁。";
            break;
        case 400:
            diagnostic = "错误请求。原因：翻译文本可能过长或包含非法字符。";
            break;
        default:
            diagnostic = QString("Google 返回了未知 HTTP 错误码: %1。网络错误: %2").arg(httpCode).arg(reply->errorString());
            break;
        }
    }

    DebugHelper::recordTreeLog("chat_translate", QString("❌ Google API 报错: %1").arg(diagnostic), 2);
}

void TranslateManager::handleBaiduTranslateError(const QString &errorCode, const QString &from, const QString &to)
{
    int code = errorCode.toInt();
    QString diagnostic = "";

    switch (code) {
    case 52001:
        diagnostic = "请求超时。建议：检查网络连接或稍后重试。";
        break;
    case 52003:
        diagnostic = "未授权用户。检查：AppID 是否填错，或者该服务在后台是否已开启。";
        break;
    case 54001:
        diagnostic = "签名错误。检查：请确保 SecretKey 填入正确，且没有多余空格。";
        break;
    case 54003:
        diagnostic = "访问频率受限 (QPS=1)。建议：前往百度后台完成实名认证可提升至 10 QPS，或代码中增加延时。";
        break;
    case 54004:
        diagnostic = "账户余额不足。建议：检查百度翻译后台是否产生欠费。";
        break;
    case 58000:
        diagnostic = "客户端 IP 非法。检查：百度后台是否设置了 IP 白名单（建议留空）。";
        break;
    case 58001:
        diagnostic = QString("语言方向不支持 [%1 -> %2]。核心原因：此语种需要您在百度后台手动点击开通'全语种'服务，且需完成个人认证。").arg(from, to);
        break;
    case 58002:
        diagnostic = "服务当前已关闭。建议：去后台手动开启通用翻译服务。";
        break;
    case 20003:
        diagnostic = "请求内容存在安全风险。原因：文本包含敏感词汇，已被百度系统拦截。";
        break;
    default:
        diagnostic = QString("百度 API 返回了未定义错误 (%1)。请参考官方错误码文档。").arg(errorCode);
        break;
    }

    DebugHelper::recordTreeLog("chat_translate", QString("❌ 百度 API 报错 [%1]: %2").arg(errorCode, diagnostic), 2);
}

QString TranslateManager::mapToGoogleCode(const QString &qtCode)
{
    // 1. 处理特殊的中文变体
    QString code = qtCode.toLower().replace("-", "_");

    if (code.startsWith("zh_cn") || code.startsWith("zh_sg") || code == "zh")
        return "zh-CN";
    if (code.startsWith("zh_tw") || code.startsWith("zh_hk") || code.startsWith("zh_mo"))
        return "zh-TW";

    // 2. 提取语言主代码
    QString langPart = code.split('_').first();

    static const QHash<QString, QString> gMap = {
        {"en", "en"}, {"ru", "ru"}, {"es", "es"}, {"de", "de"},
        {"fr", "fr"}, {"it", "it"}, {"ja", "ja"}, {"ko", "ko"},
        {"pl", "pl"}, {"pt", "pt"}, {"uk", "uk"}, {"ar", "ar"},
        {"bg", "bg"}, {"ca", "ca"}, {"cs", "cs"}, {"da", "da"},
        {"fi", "fi"}, {"gd", "gd"}, {"he", "iw"}, {"hu", "hu"},
        {"lv", "lv"}, {"sk", "sk"}, {"tr", "tr"}, {"vi", "vi"},
        {"th", "th"}, {"ms", "ms"}
    };

    return gMap.value(langPart, langPart.isEmpty() ? "zh-CN" : langPart);
}

QString TranslateManager::mapToBaiduCode(const QString &qtCode)
{
    // 1. 处理中文特殊逻辑
    QString code = qtCode.toLower().replace("-", "_");

    if (code.startsWith("zh_cn") || code.startsWith("zh_sg") || code == "zh")
        return "zh";
    if (code.startsWith("zh_tw") || code.startsWith("zh_hk") || code.startsWith("zh_mo"))
        return "cht";

    // 2. 提取语言主代码
    QString langPart = code.split('_').first();

    static const QHash<QString, QString> langMap = {
        {"en", "en"},   {"ru", "ru"},   {"es", "spa"},  {"de", "de"},
        {"fr", "fra"},  {"it", "it"},   {"ja", "jp"},   {"ko", "kor"},
        {"pl", "pl"},   {"pt", "pt"},   {"uk", "ukr"},  {"ar", "ara"},
        {"bg", "bul"},  {"ca", "cat"},  {"cs", "cs"},   {"da", "dan"},
        {"fi", "fin"},  {"gd", "gla"},  {"he", "heb"},  {"hu", "hu"},
        {"lv", "lav"},  {"sk", "sk"},   {"tr", "tr"},   {"vi", "vie"},
        {"th", "th"},   {"ms", "may"}
    };

    return langMap.value(langPart, "zh");
}
