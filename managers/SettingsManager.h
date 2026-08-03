#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>
#include <QDebug>

enum ServerName {
    CN1,
    US1,
    UNKNOWN
};

struct ShoutItem {
    QString key;
    QString src;
    QString def;
};

class SettingsManager : public QObject
{
    Q_OBJECT
    // 定义属性，使其可以在 QML 中通过 SettingsManager.languageCode 访问
    Q_PROPERTY(QString languageCode READ languageCode WRITE setLanguageCode NOTIFY languageCodeChanged)
    // 如果你之前的 main.qml 用到了这个，也加上
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    static SettingsManager &instance();

    static ServerName stringToServerName(const QString &str);
    static QString serverNameToString(ServerName serverName);

    // Getter
    QString languageCode() const { return m_languageCode; }
    bool isLoading() const { return m_isLoading; }
    QByteArray appSecret() const;
    QString getRegistryPath(const QString &key = "");
    QString getConfigFilePath(const QString &fileName = "GameConfig.ini") const;
    QString getAppDataConfigPath(const QString &fileName = "GameConfig.ini") const;
    void initializeclientId();

    // Setter
    Q_INVOKABLE void setLanguageCode(const QString &code);
    Q_INVOKABLE bool isDefaultShoutContent(const QString &content);
    Q_INVOKABLE bool isDefaultSchemeName(const QString &name) const;
    Q_INVOKABLE QString serverAddresses(quint32 index = 0) const;
    Q_INVOKABLE quint16 serverPort() const;
    Q_INVOKABLE QString hardwareId() const;
    Q_INVOKABLE QString clientId() const;

signals:
    void languageCodeChanged();
    void languageAboutToChange(); // 语言即将改变的信号
    void isLoadingChanged();

private:
    explicit SettingsManager(QObject *parent = nullptr);

    QString m_languageCode;
    bool m_isLoading;
    QSettings *m_settings; // 用于持久化保存配置
    QList<ShoutItem> m_shoutTemplate;
    QString m_clientId;
    QString m_hardwareId;
};

#endif // SETTINGSMANAGER_H