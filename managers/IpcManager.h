#ifndef IPCMANAGER_H
#define IPCMANAGER_H

#include <QTimer>
#include <QDebug>
#include <QMutex>
#include <QThread>
#include <QObject>
#include <QWinEventNotifier>

#include <windows.h>
#include "../shared/SharedMemory.h"

class IpcManager : public QObject
{
    Q_OBJECT
public:
    // 禁止拷贝和赋值
    IpcManager(const IpcManager&) = delete;
    IpcManager &operator=(const IpcManager&) = delete;

    // 获取单例实例
    static IpcManager &instance();

    Q_INVOKABLE void  resetIpc();
    Q_INVOKABLE void  cleanup();
    Q_INVOKABLE QString initIpcManager();

    QWinEventNotifier *m_eventNotifier        = nullptr;
    SharedData        *m_pSharedData          = nullptr;
    HANDLE            m_hSharedMemory         = NULL;
    HANDLE            m_hIpcEvent             = NULL;

public slots:
    void onIpcMessageReceived();

signals:
    void incomingMessageIntercepted(quint32 pid, QString sender, QString text, quint32 direction);

private:
    explicit IpcManager(QObject *parent = nullptr);
    ~IpcManager();

public:
    bool sendIpcBufferMessage(IpcMessageType type, const void* data = nullptr, quint32 size = 0);
    void dispatchIpcBufferMessage(const MessageSlot &message);
    void updateTranslateLanguage(const QString &code);
};

#endif // IPCMANAGER_H
