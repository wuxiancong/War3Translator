#include "ChatManager.h"

ChatManager &ChatManager::instance() {
    static ChatManager _instance;
    return _instance;
}

ChatManager::ChatManager(QObject *parent) : QObject(parent) {

}

void ChatManager::processGameMessage(const QString &sender, const QString &content) {
    emit messageReceived(sender, content);
}