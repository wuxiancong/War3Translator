#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "../managers/SettingsManager.h"
#include "../shared/SharedMemory.h"

#include <QAtomicInteger>
#include <QHostAddress>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QAtomicInt>
#include <QDateTime>
#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QMap>

const int PUNCH_TIMER_INTERVAL_MS = 200;
const int MAX_PUNCH_PHASE_TIME_MS = 1500;
const int MAX_TEST_PHASE_TIME_MS = 3000;
const int P2P_KEEP_ALIVE_TIMEOUT_MS = 30000;
const int MAX_SESSION_LIFETIME_MS = 6000;
const int MAX_PUNCH_COUNT = MAX_PUNCH_PHASE_TIME_MS / PUNCH_TIMER_INTERVAL_MS;

enum RegistrationState {
    Unregistered,
    Registering,
    Registered,
    Failed
};

struct LocalAddressInfo {
    QString ip;
    quint16 port = 0;
    bool valid = false;

    LocalAddressInfo() = default;
    LocalAddressInfo(const QString& ip, quint16 port, bool valid = true)
        : ip(ip), port(port), valid(valid) {}
};

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    // 禁止拷贝和赋值
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;

    // 获取单例实例
    static NetworkManager &instance();

    // 监听和退出
    bool initNetworkManager();
    Q_INVOKABLE void stopListening();
    Q_INVOKABLE void gracefulExit();
    Q_INVOKABLE void handleChatCommand(const QString &fullText, const QString &userName);
    Q_INVOKABLE void sendCommandMessage(const QString &userName, const QString &command, const QString &text);
    Q_INVOKABLE void sendTranslatedMessage(quint32 pid, quint32 flag, quint32 extraScope, const QString &translatedMessage);

    // 服务器交互
    bool pingToUDPServer();
    void registerToServer();
    void unregisterToServer();
    void connectToTcpServer();
    void setupServer(ServerName serverName);

    // 地址获取
    LocalAddressInfo getLocalAddressInfo();
    QHostAddress getLocalAddress() const;

    // 状态查询
    bool isListening() const;
    bool isHeartbeatRunning() const;
    bool isServerConnected() const;
    bool isFastMessage(IpcMessageType type) const;
    RegistrationState getRegistrationState() const;

    // 工具函数
    QString getNetworkInterfaceInfo() const;
    QString getCurrentServerAddress() const;

signals:
    void networkDisconnected();
    void unhostResponseReceived();
    void unregisteredStateDetected();
    void botChanged(ServerName serverName);
    void tcpStatusChanged(bool connected, const QString &statusText);
    void dataReceived(const QByteArray &data, const QHostAddress &fromAddr, quint16 fromPort);
    void udpConnectionStatusChanged(RegistrationState registrationState);
    void networkError(const QString &error);
    void registrationStatus(bool success, const QString& details);
    void unregistrationStatus(bool success);

private:
    explicit NetworkManager(QObject  *parent = nullptr);
    ~NetworkManager();

    // 初始化相关
    void initializeTimers();
    void initializeServer();
    void setupConnections();

    // Socket管理
    bool createAndBindPublicSocket();
    bool ensurePublicSocketCreated();
    bool isGameSocketValid() const;
    bool isPublicSocketValid() const;
    void cleanupExistingGameSocket();
    void cleanupExistingPublicSocket();
    void setupGameSocketConnections();
    void setupPublicSocketConnections();
    void configureGameSocketOptions();
    void configurePublicSocketOptions();

    // 网络服务管理
    void cleanupExistingTcpSocket();
    void resetConnectionState();
    void startNetworkTimers();
    void cleanupTimers();
    void safeCleanup();

    // 发送通用函数
    quint64 sendUdpPacket(PacketType type, const void *payloadData = nullptr, quint16 payloadSize = 0);
    quint64 sendTcpPacket(PacketType type, const void *payloadData = nullptr, quint16 payloadSize = 0);
    QByteArray preparePacket(PacketType type, const void *payloadData, quint16 payloadSize);
    quint64 sendUdpRaw(const QByteArray &data, const QHostAddress &address, quint16 port);
    quint64 sendTcpRaw(const QByteArray &data);

    // 辅助函数
    quint16 calculateCRC16(const QByteArray &data);

    // 接收处理
    void handleIncomingDatagram(const QNetworkDatagram &datagram);
    // === CMD 响应处理器 ===

    void handleServerCommand(const QString &type, const QString &data);
    void handleRegisterResponse(const SCRegisterPacket *packet);
    void handlePongResponse(const SCPongPacket* packet);

    // 地址获取
    quint16 getPublicPort() const;
    QString getPublicAddress() const;
    quint16 getLocalPort() const;

    // 心跳机制
    void startHeartbeat();
    void stopHeartbeat();

    // 工具函数
    QString getRegistrationStateString(RegistrationState state) const;

    // 数据转发
    bool sendIpcBufferMessage(IpcMessageType type, const QString &data);
    bool sendIpcBufferMessage(IpcMessageType type, const void *pData, size_t dataSize);

    // 消息分发
    void dispatchIpcBufferMessage(const MessageSlot &message, const QString &userName, const QString &clientId);

private slots:
    void onTcpHeartbeatTimeout();
    void onUdpHeartbeatTimeout();
    void onRegistrationTimeout();

    // TCP连接事件处理
    void onTcpConnected();
    void onTcpReadyRead();
    void onTcpDisconnected();
    void onTcpError(QAbstractSocket::SocketError socketError);
    void onTcpStateChanged(QAbstractSocket::SocketState socketState);

    // 状态检测
    void onUnregisteredStateDetected();
    void onUdpConnectionStatusChanged(RegistrationState registrationState);

    // Socket事件处理
    void onPublicSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketStateChanged(QAbstractSocket::SocketState state) const;

    // 翻译处理
    void onTranslationFinished(quint32 pid, quint32 flag, quint32 extraScope, QString originalMessage, QString translatedMessage);
private:
    // Socket相关
    QTcpSocket *m_tcpSocket;
    QUdpSocket *m_gameSocket;
    QUdpSocket *m_publicSocket;
    mutable QMutex m_socketMutex;

    // NAT 相关
    quint16 m_relayPort;
    QString m_turnRealm;
    quint16 m_relayedPort;
    QString m_turnUsername;
    QString m_turnPassword;
    QByteArray m_turnNonce;
    quint32 m_turnLifetime;
    QTimer *m_turnRefreshTimer;
    QByteArray m_transactionId;
    QHostAddress m_relayAddress;
    QHostAddress m_relayedAddress;

    // 服务器配置
    QHostAddress m_serverAddress;
    quint16 m_serverPort;

    // 状态标志
    QAtomicInteger<bool> m_heartbeatRunning;
    QAtomicInteger<bool> m_isListening;

    // 定时器
    QTimer *m_tcpHeartbeatTimer;
    QTimer *m_udpHeartbeatTimer;
    QTimer *m_registrationTimer;
    QTimer *m_networkCheckTimer;

    // 网络状态
    QString m_detectedPublicIp;
    quint16 m_detectedPublicPort;
    RegistrationState m_registrationState;

    // 连接状态
    std::atomic<quint64> m_tcpLastPingTime;
    std::atomic<quint64> m_tcpLastPongTime;
    std::atomic<quint64> m_udpLastPingTime;
    std::atomic<quint64> m_udpLastPongTime;
    std::atomic<int> m_tcpConsecutiveFailures;
    std::atomic<int> m_udpConsecutiveFailures;

    // 统计信息
    quint64 m_totalBytesSent;
    quint64 m_totalBytesReceived;
    quint64 m_totalPacketsSent;
    quint64 m_totalPacketsReceived;

    // 共享内存
    HANDLE  m_hSharedMemory;
    QString m_lastIpcUsername;
    QString m_lastIpcUserFrom;

    // 二进制协议状态
    quint32 m_sessionId;    // Session ID
    quint64 m_localSeq;     // 本地发送序列号

    // 聊天命令
    QString m_lastHostCommand;
    QString m_lastHostUserName;

    // 错误信息
    QString m_lastErrorText;
    int m_lastErrorCode;

    // 加入防重
    QString m_lastPreJoinUser;
    QString m_lastPreJoinRoom;
    qint64 m_lastPreJoinTime = 0;

    // 启动有关
    bool m_isLaunchProtecting;
    QStringList m_lockedPlayerList;
    QMap<QString, uint8_t> m_maxProgressMap;
    QMap<QString, quint16> m_lastStateCodeMap;

    // 自定义界面
    qint64 m_lastCustomUIMessageTime = 0;
};

#endif // NETWORKMANAGER_H
