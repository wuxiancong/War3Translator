#include "SingleApplication.h"

SingleApplication::SingleApplication(int &argc, char **argv, const QString &serverName)
    : QApplication(argc, argv)
    , m_localServer(nullptr)
    , m_isRunning(false)
{
    m_serverName = serverName;
    initLocalConnection();
}

bool SingleApplication::isRunning()
{
    return m_isRunning;
}

void SingleApplication::newLocalConnection()
{
    // 有新连接时，激活现有窗口
    emit instanceStarted();
}

void SingleApplication::closeInstance() {
    if (m_localServer) {
        m_localServer->close();
        QLocalServer::removeServer(m_serverName);
        m_localServer->deleteLater();
        m_localServer = nullptr;
    }
}

void SingleApplication::initLocalConnection()
{
    QLocalSocket socket;
    socket.connectToServer(m_serverName);

    if (socket.waitForConnected(500)) {
        m_isRunning = true;
        return;
    }

    QLocalServer::removeServer(m_serverName);
    newLocalServer();
}

void SingleApplication::newLocalServer()
{
    m_localServer = new QLocalServer(this);
    connect(m_localServer, &QLocalServer::newConnection,
            this, &SingleApplication::newLocalConnection);

    // 移除可能存在的旧服务器
    QLocalServer::removeServer(m_serverName);
    if (!m_localServer->listen(m_serverName)) {
        qDebug() << "无法创建本地服务器:" << m_localServer->errorString();
    }
}
