#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QSettings>
#include <QObject>
#include <QString>
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
    Q_PROPERTY(quint32 translateSendInterval READ translateSendInterval WRITE setTranslateSendInterval NOTIFY translateSendIntervalChanged)
    Q_PROPERTY(QStringList translateLanguages READ translateLanguages WRITE setTranslateLanguages NOTIFY translateLanguagesChanged)
    Q_PROPERTY(QString translateLanguage READ translateLanguage WRITE setTranslateLanguage NOTIFY translateLanguageChanged)
    Q_PROPERTY(QString languageCode READ languageCode WRITE setLanguageCode NOTIFY languageCodeChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    static SettingsManager &instance();

    static ServerName stringToServerName(const QString &str);
    static QString serverNameToString(ServerName serverName);

    // Getter
    bool isLoading() const { return m_isLoading; }
    QString languageCode() const { return m_languageCode; }
    QString translateLanguage() const { return m_translateLanguage; }
    QStringList translateLanguages() const { return m_translateLanguages; }
    quint32 translateSendInterval() const { return m_translateSendInterval; }
    QByteArray appSecret() const;
    QString getRegistryPath(const QString &key = "");
    QString getConfigFilePath(const QString &fileName = "GameConfig.ini") const;
    QString getAppDataConfigPath(const QString &fileName = "GameConfig.ini") const;
    void saveConfigToAllEnds(const QString &groupAndKey, const QVariant &value);
    void initializeclientId();

    // Setter
    Q_INVOKABLE static QString getLanguageName(const QString &code);
    Q_INVOKABLE void setLanguageCode(const QString &code);
    Q_INVOKABLE void setTranslateLanguage(const QString &code);
    Q_INVOKABLE void setTranslateLanguages(const QStringList &languages);
    Q_INVOKABLE void setTranslateSendInterval(quint32 interval);
    Q_INVOKABLE bool isDefaultShoutContent(const QString &content);
    Q_INVOKABLE bool isDefaultSchemeName(const QString &name) const;
    Q_INVOKABLE QString serverAddresses(quint32 index = 0) const;
    Q_INVOKABLE quint16 serverPort() const;
    Q_INVOKABLE QString hardwareId() const;
    Q_INVOKABLE QString clientId() const;

signals:
    void isLoadingChanged();
    void languageCodeChanged();
    void languageAboutToChange();
    void translateLanguageChanged();
    void translateLanguagesChanged();
    void translateSendIntervalChanged();

private:
    explicit SettingsManager(QObject *parent = nullptr);

    bool m_isLoading;
    QString m_clientId;
    QString m_hardwareId;
    QSettings *m_settings;
    QString m_languageCode;
    QString m_translateLanguage;
    QStringList m_translateLanguages;
    quint32 m_translateSendInterval;
    QList<ShoutItem> m_shoutTemplate;
};

#endif // SETTINGSMANAGER_H