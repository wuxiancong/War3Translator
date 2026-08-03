#ifndef CHATMANAGER_H
#define CHATMANAGER_H

#include <QObject>
#include <QString>

class ChatManager : public QObject
{
    Q_OBJECT
public:
    static ChatManager& instance();

    void processGameMessage(const QString &sender, const QString &content);

signals:
    void messageReceived(QString sender, QString text);

private:
    explicit ChatManager(QObject *parent = nullptr);
    Q_DISABLE_COPY(ChatManager)
};

#endif // CHATMANAGER_H