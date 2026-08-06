#ifndef TRANSLATEMANAGER_H
#define TRANSLATEMANAGER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>
#include <QQueue>
#include <QTimer>

class TranslateManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isProcessing READ isProcessing NOTIFY isProcessingChanged)

public:
    // 禁止拷贝和赋值
    TranslateManager(const TranslateManager&) = delete;
    TranslateManager& operator=(const TranslateManager&) = delete;

    // 获取单例实例
    static TranslateManager &instance();

    void cleanup();
    void initTranslateManager();
    bool isProcessing() const { return m_isProcessing; }

    Q_INVOKABLE void requestTranslationWithMetadata(quint32 pid, quint32 flag, quint32 extraScope, quint32 direction, QString message, QString language);
    Q_INVOKABLE void requestAllTranslations(const QString &sourceText, const QString &sourceLangCode);
    Q_INVOKABLE QString translateSingleSync(const QString &sourceText, const QString &targetLangCode);
    Q_INVOKABLE QString getTranslation(const QString &sourceText, const QString &targetLangCode);
    Q_INVOKABLE quint8 getTranslationCount(const QString &sourceText);
    Q_INVOKABLE void cancelTasksForText(const QString &text);

signals:
    void cacheUpdated();
    void isProcessingChanged();
    void translationTaskFinished(quint32 pid, quint32 flag, quint32 extraScope, quint32 direction, QString originalMessage, QString translatedMessage, QString language);

private:
    explicit TranslateManager(QObject *parent = nullptr);
    ~TranslateManager();

    struct TranslationTask {
        QString originalText;
        QString fromLang;
        QString toLang;
        int retryCount = 0;
    };

    void loadCache();
    void saveCache();
    void processNextTask();
    bool loadFromRegistry();
    void limitCacheSize(int maxEntries = 2000);
    void saveToRegistry(const QString &jsonStr);
    void handleGoogleError(QNetworkReply *reply);
    QString mapToBaiduCode(const QString &qtCode);
    QString mapToGoogleCode(const QString &qtCode);
    bool loadFromFile(const QString &path, const QString &label);
    void saveToFile(const QString &path, const QByteArray &data, const QString &label);
    void handleBaiduTranslateError(const QString &errorCode, const QString &from, const QString &to);
    void updateDbAndNotify(const QString &originalText, const QString &qtLanguageCode, const QString &translatedText);

    QNetworkAccessManager *m_networkManager;
    QQueue<TranslationTask> m_taskQueue;
    quint32 m_translateTimeout = 5000;
    QJsonObject m_translationDb;
    bool m_isProcessing = false;
    QTimer *m_apiRateTimer;

    const QString m_appId = "20260616002632996";
    const QString m_appKey = "EJUhHEj6te5ODrtjlSgC";
};

#endif // TRANSLATEMANAGER_H
