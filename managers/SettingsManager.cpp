#include "../helpers/HardwareHelper.h"
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
    // 初始化 QSettings (保存到：用户文档/YourApp/config.ini)
    m_settings = new QSettings("YourCompany", "War3Launcher", this);

    // 从本地读取上次保存的语言，默认 zh_CN
    m_languageCode = m_settings->value("base/languageCode", "zh_CN").toString();
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

void SettingsManager::setLanguageCode(const QString &code) {
    if (m_languageCode == code) return;

    qDebug() << "🌐 [C++] 切换语言为:" << code;

    // 发送“即将改变”信号（用于 UI 做一些清理工作）
    emit languageAboutToChange();

    m_languageCode = code;

    // 持久化保存到硬盘
    m_settings->setValue("base/languageCode", code);
    m_settings->sync();

    // 发送改变信号，触发 QML 的 Connections
    emit languageCodeChanged();
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