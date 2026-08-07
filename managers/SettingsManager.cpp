#include "../helpers/HardwareHelper.h"
#include "../managers/IpcManager.h"
#include "SettingsManager.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUuid>
#include <QDir>

SettingsManager &SettingsManager::instance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), m_isLoading(false)
{
    // 1. 获取三端路径
    QString localPath   = getConfigFilePath("GameConfig.ini");
    QString appDataPath = getAppDataConfigPath("GameConfig.ini");
    QString regKeyPath  = getRegistryPath("");

    // 2. 三段式读取逻辑 (本地 .ini -> AppData .ini -> 注册表)
    auto readChain = [&](const QString &groupAndKey, const QVariant &defaultValue) -> QVariant {
        // 第一步：尝试从本地 exe 同级 ini 读取
        if (QFile::exists(localPath)) {
            QSettings local(localPath, QSettings::IniFormat);
            if (local.contains(groupAndKey)) return local.value(groupAndKey);
        }
        // 第二步：尝试从 AppData 目录 ini 读取
        if (QFile::exists(appDataPath)) {
            QSettings appData(appDataPath, QSettings::IniFormat);
            if (appData.contains(groupAndKey)) return appData.value(groupAndKey);
        }
        // 第三步：尝试从 Windows 注册表读取
        QSettings reg(regKeyPath, QSettings::NativeFormat);
        if (reg.contains(groupAndKey)) {
            return reg.value(groupAndKey);
        }
        // 都找不到，返回默认值
        return defaultValue;
    };

    // 3. 加载语言配置
    m_languageCode          = readChain("Base/languageCode", "zh_CN").toString();
    m_translateLanguage     = readChain("Base/translateLanguage", "zh_CN").toString();
    m_translateLanguages    = readChain("Base/translateLanguages", QStringList() << "zh_CN").toStringList();
    m_translateSendInterval = readChain("Base/translateSendInterval", 1500).toUInt();

    // 4. 保持你原本的主实例化指针指向本地
    m_settings = new QSettings(localPath, QSettings::IniFormat, this);

    // 5. 调用你原本的客户端 ID 初始化
    initializeclientId();
}

void SettingsManager::initializeclientId()
{
    QString localPath = getConfigFilePath();
    QString appDataPath = getAppDataConfigPath();
    QString regClientPath = getRegistryPath("Client");

    // --- 1. 读取逻辑 (本地 -> 注册表 -> AppData) ---
    auto loadFromIni = [](const QString& path) -> QString {
        if (!QFile::exists(path)) return "";
        QSettings settings(path, QSettings::IniFormat);
        return settings.value("Client/uuid").toString();
    };

    m_clientId = loadFromIni(localPath);
    if (m_clientId.isEmpty()) {
        QSettings settings(regClientPath, QSettings::NativeFormat);
        m_clientId = settings.value("uuid").toString();
    }
    if (m_clientId.isEmpty()) m_clientId = loadFromIni(appDataPath);

    // --- 2. 生成逻辑 ---
    if (m_clientId.isEmpty()) {
        m_clientId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    m_hardwareId = HardwareHelper::instance().generateHardwareID();

    // --- 3. 写入逻辑 ---

    // A. 写入注册表
    QSettings regSettings(regClientPath, QSettings::NativeFormat);
    regSettings.setValue("uuid", m_clientId);
    regSettings.setValue("hwid", m_hardwareId);
    regSettings.sync();

    // B. 写入本地 INI
    QSettings localSettings(localPath, QSettings::IniFormat);
    localSettings.setIniCodec("UTF-8");
    localSettings.beginGroup("Client");
    localSettings.setValue("uuid", m_clientId);
    localSettings.setValue("hwid", m_hardwareId);
    localSettings.endGroup();
    localSettings.sync();

    // C. 写入 AppData INI
    QSettings appSettings(appDataPath, QSettings::IniFormat);
    appSettings.setIniCodec("UTF-8");
    appSettings.beginGroup("Client");
    appSettings.setValue("uuid", m_clientId);
    appSettings.setValue("hwid", m_hardwareId);
    appSettings.endGroup();
    appSettings.sync();

    qDebug() << "🛡 软件/硬件ID已通过 RegisterManager 风格完成全量同步";
}

void SettingsManager::saveConfigToAllEnds(const QString &groupAndKey, const QVariant &value)
{
    QString localPath   = getConfigFilePath("GameConfig.ini");
    QString appDataPath = getAppDataConfigPath("GameConfig.ini");
    QString regKeyPath  = getRegistryPath("");

    // 1. 写入本地 INI
    QSettings localSettings(localPath, QSettings::IniFormat);
    localSettings.setValue(groupAndKey, value);
    localSettings.sync();

    // 2. 写入 AppData INI
    QSettings appSettings(appDataPath, QSettings::IniFormat);
    appSettings.setValue(groupAndKey, value);
    appSettings.sync();

    // 3. 写入注册表
    QSettings regSettings(regKeyPath, QSettings::NativeFormat);
    regSettings.setValue(groupAndKey, value);
    regSettings.sync();
}

QString SettingsManager::getLanguageName(const QString &code)
{
    static const QMap<QString, QString> langMap = {
        {"zh_CN", tr("简体中文")},
        {"zh_TW", tr("繁體中文")},
        {"en",    tr("英语")},
        {"ru",    tr("俄语")},
        {"es",    tr("西班牙语")},
        {"de",    tr("德语")},
        {"fr",    tr("法语")},
        {"it",    tr("意大利语")},
        {"ja",    tr("日语")},
        {"ko",    tr("韩语")},
        {"pl",    tr("波兰语")},
        {"pt",    tr("葡萄牙语")},
        {"uk",    tr("乌克兰语")},
        {"ar",    tr("阿拉伯语")},
        {"bg",    tr("保加利亚语")},
        {"ca",    tr("加泰罗尼亚语")},
        {"cs",    tr("捷克语")},
        {"da",    tr("丹麦语")},
        {"fi",    tr("芬兰语")},
        {"gd",    tr("苏格兰盖尔语")},
        {"he",    tr("希伯来语")},
        {"hu",    tr("匈牙利语")},
        {"lv",    tr("拉脱维亚语")},
        {"sk",    tr("斯洛伐克语")},
        {"tr",    tr("土耳其语")}
    };

    return langMap.value(code, code);
}

void SettingsManager::setLanguageCode(const QString &code) {
    if (m_languageCode == code) return;

    qDebug() << "🌐 [C++] 切换语言为:" << code;

    // 发送即将改变信号
    emit languageAboutToChange();

    m_languageCode = code;

    // 全量持久化同步保存到三端硬盘/注册表
    saveConfigToAllEnds("Base/languageCode", code);

    // 发送改变信号，触发 QML 的 Connections
    emit languageCodeChanged();
}

void SettingsManager::setTranslateLanguage(const QString &code) {
    if (m_translateLanguage == code) return;

    qDebug() << "🎯 翻译目标语言切换为:" << code;

    m_translateLanguage = code;

    // 全量持久化同步保存到三端硬盘/注册表
    saveConfigToAllEnds("Base/translateLanguage", code);

    // 同步到共享内存，让游戏内的 DLL 知道要把聊天翻译成什么
    IpcManager::instance().updateTranslateLanguage(code);

    // 通知 QML 更新 UI
    emit translateLanguageChanged();
}

void SettingsManager::setTranslateLanguages(const QStringList &languages)
{
    if (m_translateLanguages.size() == languages.size()) {
        bool allSame = true;
        for(int i = 0; i < languages.size(); ++i) {
            if(m_translateLanguages.at(i) != languages.at(i)) {
                allSame = false;
                break;
            }
        }
        if (allSame) return;
    }

    if (IpcManager::instance().m_pSharedData) {
        auto *pSharedData = IpcManager::instance().m_pSharedData;

        // 1. 更新数量
        uint32_t count = qMin((uint32_t)languages.size(), (uint32_t)MAX_TARGET_LANGUAGES);
        pSharedData->translate_languages_count = count;

        // 2. 清理并填充数组
        memset(pSharedData->translate_languages, 0, sizeof(pSharedData->translate_languages));
        for (uint32_t i = 0; i < count; ++i) {
            QByteArray ba = languages.at(i).toUtf8();
            strncpy_s(pSharedData->translate_languages[i], ba.constData(), 7);
        }

        qDebug() << "📢 [IPC] 已同步多语言发送列表，数量:" << count;
    }

    qDebug() << "📝 更新发送语种列表:" << languages;

    m_translateLanguages = languages;

    // 同步到三端持久化
    saveConfigToAllEnds("Base/translateLanguages", languages);

    emit translateLanguagesChanged();
}

void SettingsManager::setTranslateSendInterval(quint32 interval)
{
    if (m_translateSendInterval == interval) return;
    if (interval < MIN_SEND_INTERVAL) interval = MIN_SEND_INTERVAL;

    qDebug() << "⏱ 翻译发送间隔更新为:" << interval << "ms";

    m_translateSendInterval = interval;

    saveConfigToAllEnds("Base/translateSendInterval", interval);

    if (IpcManager::instance().m_pSharedData) {
        IpcManager::instance().m_pSharedData->translate_send_interval = interval;
        qDebug() << "📢 [IPC] 发送间隔已同步至共享内存";
    }

    emit translateSendIntervalChanged();
}

bool SettingsManager::isDefaultShoutContent(const QString &content)
{
    for (const auto &item : qAsConst(m_shoutTemplate)) {
        if (content == item.src || content == item.def) {
            return true;
        }
    }
    return false;
}

bool SettingsManager::isDefaultSchemeName(const QString &name) const
{
    const QString anchor = "Dota常用喊话";
    if (name == anchor || name == tr("Dota常用喊话")) {
        return true;
    }
    return false;
}

QString SettingsManager::getConfigFilePath(const QString &fileName) const
{
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath(fileName);
    qDebug() << "配置文件路径:" << configPath;
    return configPath;
}

QString SettingsManager::getAppDataConfigPath(const QString &fileName) const
{
    QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (appDataDir.isEmpty()) {
        appDataDir = QDir::cleanPath(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                                     + "/" + QCoreApplication::applicationName());
    }

    QDir dir(appDataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
        qDebug() << "📁 [System] 已自动创建不存在的 AppData 目录:" << appDataDir;
    }

    return dir.filePath(fileName);
}

QString SettingsManager::getRegistryPath(const QString &key)
{
    QString appName = QCoreApplication::applicationName();
    if (appName.isEmpty()) {
        appName = "War3Translator";
    }
    QString path = QString("HKEY_CURRENT_USER\\Software\\%1").arg(appName);

    if (!key.isEmpty()) {
        path += "\\" + key;
    }

    return path;
}

QString SettingsManager::serverAddresses(quint32 index) const
{
    QString address = "139.155.155.166";
    switch (index) {
    case CN1:
        address = "139.155.155.166";
        break;
    case US1:
        address = "212.192.15.24";
        break;
    default:
        address = "139.155.155.166";
        break;
    }
    return address;
}

quint16 SettingsManager::serverPort() const
{
    return 6116;
}

QString SettingsManager::serverNameToString(ServerName serverName)
{
    static const QMap<ServerName, QString> enumToName = {
        {CN1, "CN1"},
        {US1, "US1"}
    };
    return enumToName.value(serverName, "UNKNOWN");
}

ServerName SettingsManager::stringToServerName(const QString &str)
{
    static const QMap<QString, ServerName> nameToEnum = {
        {"CN1", CN1},
        {"US1", US1}
    };

    QString key = str.toUpper().trimmed();
    return nameToEnum.value(key, UNKNOWN);
}

QByteArray SettingsManager::appSecret() const
{
    QByteArray part1 = "CC_War3_";
    QByteArray part2 = "@#_Plat";
    QByteArray part3 = "form_2026";
    QByteArray part4 = "_SecureKey";

    // CC_War3_@#_Platform_2026_SecureKey
    return part1 + part2 + part3 + part4;
}

QString SettingsManager::hardwareId() const
{
    return m_hardwareId;
}

QString SettingsManager::clientId() const
{
    return m_clientId;
}