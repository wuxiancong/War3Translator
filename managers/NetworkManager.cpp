#include "../managers/TranslateManager.h"
#include "../managers/NetworkManager.h"
#include "../managers/IpcManager.h"
#include "../helpers/DebugHelper.h"
#include <QCryptographicHash>
#include <QNetworkInterface>
#include <QNetworkDatagram>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QNetworkProxy>
#include <QDataStream>
#include <QSettings>
#include <QDateTime>
#include <QHostInfo>
#include <QPointer>
#include <QThread>
#include <QDebug>
#include <QDir>

NetworkManager &NetworkManager::instance()
{
    static NetworkManager instance;
    return instance;
}

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_gameSocket(nullptr)
    , m_publicSocket(nullptr)
    , m_serverPort(6116)
    , m_isListening(false)
    , m_heartbeatRunning(false)
    , m_registrationState(Unregistered)
    , m_detectedPublicPort(0)
    , m_udpLastPingTime(0)
    , m_udpLastPongTime(0)
    , m_udpConsecutiveFailures(0)
    , m_totalBytesSent(0)
    , m_totalBytesReceived(0)
    , m_totalPacketsSent(0)
    , m_totalPacketsReceived(0)
    , m_sessionId(0)
    , m_localSeq(0)
{

}

NetworkManager::~NetworkManager()
{
    stopListening();
    if (m_hSharedMemory) {
        UnmapViewOfFile(IpcManager::instance().m_pSharedData);
        CloseHandle(m_hSharedMemory);
    }
}

// ==================== 初始化函数 ====================

void NetworkManager::initializeServer()
{
    // 1. 定义优先级路径：本地 -> 注册表 -> AppData
    QStringList paths = {
        SettingsManager::instance().getConfigFilePath(),
        SettingsManager::instance().getRegistryPath(),
        SettingsManager::instance().getAppDataConfigPath()
    };

    QString savedServerName;
    bool found = false;

    // 2. 依次尝试读取
    for (int i = 0; i < paths.size(); ++i) {
        bool isReg = (i == 1);
        QSettings settings(paths[i], isReg ? QSettings::NativeFormat : QSettings::IniFormat);
        if (!isReg) settings.setIniCodec("UTF-8");

        savedServerName = settings.value("Bot/name").toString();
        if (!savedServerName.isEmpty()) {
            found = true;
            qDebug() << "🔍 [Network] 从" << (isReg ? "注册表" : (i == 0 ? "本地" : "AppData")) << "加载服务器配置:" << savedServerName;
            break;
        }
    }

    // 3. 兜底逻辑
    if (!found) {
        savedServerName = "CN1";
        qDebug() << "ℹ️ [Network] 未发现服务器配置，使用默认值: CN1";
    }

    ServerName serverName = SettingsManager::stringToServerName(savedServerName);

    // 4. 后续初始化逻辑
    m_serverAddress = QHostAddress(SettingsManager::instance().serverAddresses(serverName));
    m_serverPort = SettingsManager::instance().serverPort();

    if (!m_tcpSocket) {
        m_tcpSocket = new QTcpSocket(this);
        m_tcpSocket->setProxy(QNetworkProxy::NoProxy);
    }

    qDebug() << "✅ 服务器" << savedServerName << "地址初始化完成:" << m_serverAddress.toString() << ":" << m_serverPort;
}

void NetworkManager::initializeTimers()
{
    m_udpHeartbeatTimer = new QTimer(this);
    m_udpHeartbeatTimer->setSingleShot(false);
    m_udpHeartbeatTimer->setInterval(5000);     // 5秒
    m_tcpHeartbeatTimer = new QTimer(this);
    m_tcpHeartbeatTimer->setSingleShot(false);
    m_tcpHeartbeatTimer->setInterval(15000);    // 15秒
    m_registrationTimer = new QTimer(this);
    m_registrationTimer->setSingleShot(true);
    m_networkCheckTimer = new QTimer(this);
    m_networkCheckTimer->setInterval(60000);    // 60秒
}

void NetworkManager::setupServer(ServerName serverName)
{
    m_serverAddress = QHostAddress(SettingsManager::instance().serverAddresses(serverName));
    QString nameStr = SettingsManager::serverNameToString(serverName);

    auto syncServerSetting = [&](const QString& path, bool isReg) {
        QSettings s(path, isReg ? QSettings::NativeFormat : QSettings::IniFormat);
        if (!isReg) s.setIniCodec("UTF-8");
        s.setValue("Bot/name", nameStr);
        s.sync();
    };

    syncServerSetting(SettingsManager::instance().getConfigFilePath(), false);
    syncServerSetting(SettingsManager::instance().getRegistryPath(), true);
    syncServerSetting(SettingsManager::instance().getAppDataConfigPath(), false);

    qDebug() << "🚀 [Network] 服务器节点已同步保存并准备切换:" << nameStr;

    if (m_tcpSocket) m_tcpSocket->abort();
    connectToTcpServer();
}

void NetworkManager::setupConnections()
{
    qDebug().noquote() << "🔗 [系统信号链路组装]";

    bool c = connect(&TranslateManager::instance(), &TranslateManager::translationTaskFinished,
            this, &NetworkManager::onTranslationFinished);

    qDebug().noquote() << QString("   ├─ 📡 [翻译模块] PublicAddr(%1)").arg(c ? "✅" : "❌");

    // ==================== 3. Timers (定时任务) ====================
    // TCP 心跳
    bool t1 = connect(m_tcpHeartbeatTimer, &QTimer::timeout, this, &NetworkManager::onTcpHeartbeatTimeout);
    // UDP 心跳
    bool t2 = connect(m_udpHeartbeatTimer, &QTimer::timeout, this, &NetworkManager::onUdpHeartbeatTimeout);
    // 注册超时
    bool t3 = connect(m_registrationTimer, &QTimer::timeout, this, &NetworkManager::onRegistrationTimeout);

    qDebug().noquote() << QString("   ├─ ⏰ [定时任务] TCP心跳(%1) | UDP心跳(%2) | 注册超时(%3) | 网络检测(%4)")
                              .arg(t1 ? "✅" : "❌", t2 ? "✅" : "❌", t3 ? "✅" : "❌");

    // ==================== 4. TCP Socket (传输层) ====================
    bool s1 = connect(m_tcpSocket, &QTcpSocket::connected, this, &NetworkManager::onTcpConnected);
    bool s2 = connect(m_tcpSocket, &QTcpSocket::disconnected, this, &NetworkManager::onTcpDisconnected);
    bool s3 = connect(m_tcpSocket, &QTcpSocket::readyRead, this, &NetworkManager::onTcpReadyRead);
    bool s4 = connect(m_tcpSocket, &QTcpSocket::stateChanged, this, &NetworkManager::onTcpStateChanged);
    bool s5 = connect(m_tcpSocket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onTcpError);

    qDebug().noquote() << QString("   ├─ 🔌 [TCP 链路] Connect(%1) | Disconnect(%2) | ReadyRead(%3) | StateChanged(%4) | Error(%5)")
                              .arg(s1 ? "✅" : "❌", s2 ? "✅" : "❌", s3 ? "✅" : "❌", s4 ? "✅" : "❌", s5 ? "✅" : "❌");

    // ==================== 5. Internal Logic (内部逻辑) ====================
    // 自身状态同步信号
    bool i1 = connect(this, &NetworkManager::unregisteredStateDetected,
                      this, &NetworkManager::onUnregisteredStateDetected);
    bool i2 = connect(this, &NetworkManager::udpConnectionStatusChanged,
                      this, &NetworkManager::onUdpConnectionStatusChanged);

    // 资源销毁监控
    bool i3 = connect(m_udpHeartbeatTimer, &QObject::destroyed, this, [](){
        qDebug() << "💀 严重警告：m_udpHeartbeatTimer 对象被销毁了！";
    });

    qDebug().noquote() << QString("   └─ 🔄 [内部状态]  注册状态(%1) 状态同步(%2) 资源监控(%3)")
                              .arg(i1 ? "✅" : "❌", i2 ? "✅" : "❌", i3 ? "✅" : "❌");
}

// ==================== Socket管理 ====================

bool NetworkManager::createAndBindPublicSocket()
{
    QMutexLocker locker(&m_socketMutex);
    if (m_publicSocket && m_publicSocket->isValid()) return true;
    cleanupExistingPublicSocket();

    m_publicSocket = new QUdpSocket(this);
    setupPublicSocketConnections();
    configurePublicSocketOptions();

    // 尝试绑定
    if (m_publicSocket->bind(QHostAddress::AnyIPv4, 0, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qDebug() << "✅ PublicSocket 创建并绑定到随机端口:" << m_publicSocket->localPort();
        return true;
    }

    qDebug() << "❌ PublicSocket 绑定失败:" << m_publicSocket->errorString();
    cleanupExistingPublicSocket();
    return false;
}

bool NetworkManager::isGameSocketValid() const
{
    return m_gameSocket && m_gameSocket->isValid() && m_gameSocket->state() == QAbstractSocket::BoundState;
}

bool NetworkManager::isPublicSocketValid() const
{
    return m_publicSocket && m_publicSocket->isValid() && m_publicSocket->state() == QAbstractSocket::BoundState;
}

void NetworkManager::cleanupExistingGameSocket()
{
    if (m_gameSocket) {
        m_gameSocket->close();
        m_gameSocket->deleteLater();
        m_gameSocket = nullptr;
    }
}

void NetworkManager::cleanupExistingPublicSocket()
{
    if (m_publicSocket) {
        m_publicSocket->close();
        m_publicSocket->deleteLater();
        m_publicSocket = nullptr;
    }
}

void NetworkManager::setupGameSocketConnections()
{
    if (!m_gameSocket) return;
    connect(m_gameSocket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onSocketError);
}

void NetworkManager::setupPublicSocketConnections()
{
    if (!m_publicSocket) return;
    connect(m_publicSocket, &QUdpSocket::readyRead, this, &NetworkManager::onPublicSocketReadyRead);
    connect(m_publicSocket, &QAbstractSocket::errorOccurred, this, &NetworkManager::onSocketError);
}

void NetworkManager::configureGameSocketOptions()
{
    if (!m_gameSocket) return;
    m_gameSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024);
    m_gameSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024);
    m_gameSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
}

void NetworkManager::configurePublicSocketOptions()
{
    if (!m_publicSocket) return;
    m_publicSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024);
    m_publicSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024);
    m_publicSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
}

bool NetworkManager::ensurePublicSocketCreated()
{
    return isPublicSocketValid() ? true : createAndBindPublicSocket();
}

// ==================== 核心网络功能 ====================

void NetworkManager::connectToTcpServer()
{
    if (!m_tcpSocket) {
        qDebug().noquote() << "   └─ ❌ 错误: TCP Socket 未初始化";
        return;
    }

    if (m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug().noquote() << "   └─  ✅ TCP 已连接，无需重复连接。";
        return;
    }

    qDebug() << "🔄 [TCP] 正在连接服务端..." << m_serverAddress.toString() << ":" << m_serverPort;

    m_tcpSocket->connectToHost(m_serverAddress, m_serverPort);
}

bool NetworkManager::initNetworkManager() {
    if (m_isListening.loadRelaxed()) return true;

    qDebug() << "🧵 initNetworkManager 运行在线程:" << QThread::currentThread();

    // 1. 初始化
    initializeServer();
    initializeTimers();
    setupConnections();

    // 2. 检查并绑定
    if(!ensurePublicSocketCreated()) {
        emit networkError("Socket 创建失败");
        return false;
    }

    // 3. 启动定时器
    startNetworkTimers();
    m_isListening.storeRelaxed(true);
    m_heartbeatRunning.storeRelaxed(true);

    return true;
}

bool NetworkManager::isFastMessage(IpcMessageType type) const
{
    switch (type) {
    case MSG_TYPE_TRANSLATE_REQUEST:
        return true;

    default:
        return false;
    }
}

bool NetworkManager::sendIpcBufferMessage(IpcMessageType type, const QString &data)
{
    const QString &logFile = "send_ipc_buffer_1";
    IpcManager& ipc = IpcManager::instance();

    // 1. 检查共享内存指针
    if (!ipc.m_pSharedData) {
        static qint64 lastIpcWarn = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastIpcWarn > 2000) {
            qWarning().noquote() << "❌ [IPC 严重错误] 共享内存尚未初始化";
            lastIpcWarn = now;
        }
        DebugHelper::recordTreeLog(logFile, QString("❌ IPC 失败: 共享内存未就绪"), 1, true);
        return false;
    }

    auto *sharedData = ipc.m_pSharedData;

    // 2. 准备数据并校验长度
    QByteArray dataBytes = data.toUtf8();
    if (dataBytes.size() >= MAX_PACKET_SIZE) {
        qWarning() << "❌ [IPC] 指令过长，无法发送 (Size:" << dataBytes.size() << ")";
        return false;
    }

    // 3. 优先级路由逻辑
    bool isFast = isFastMessage(type);

    // 根据优先级指向不同的索引和缓冲区
    volatile LONG* pWriteIdx = isFast ? &sharedData->fast_write_index_l2d : &sharedData->slow_write_index_l2d;
    volatile LONG* pReadIdx  = isFast ? &sharedData->fast_read_index_l2d  : &sharedData->slow_read_index_l2d;
    MessageSlot*   pBuffer   = isFast ? sharedData->fast_buffer_l2d       : sharedData->slow_buffer_l2d;

    // 4. 负载监控与满位检查
    LONG r = *pReadIdx;
    LONG w = *pWriteIdx;

    int currentUsage = (w >= r) ? (w - r) : (BUFFER_SIZE - r + w);
    int usagePercent = (currentUsage * 100) / BUFFER_SIZE;

    if (usagePercent >= 80) {
        qWarning() << QString("⚠ [IPC %1 Load] 负载过高! %2/%3 (%4%) | 类型: %5")
                          .arg(isFast ? "FAST" : "SLOW").arg(currentUsage).arg(BUFFER_SIZE).arg(usagePercent).arg(type);
    }

    LONG next_write = (w + 1) % BUFFER_SIZE;
    if (next_write == r) {
        qWarning() << QString("🔥 [IPC %1 Overload] 缓冲区满！丢弃类型: %2")
                          .arg(isFast ? "FAST" : "SLOW").arg(type);
        DebugHelper::recordTreeLog(logFile, "❌ IPC 失败: 缓冲区溢出", 1, true);
        return false;
    }

    // 5. 执行写入
    MessageSlot &slot = pBuffer[w];
    slot.type = type;
    slot.size = dataBytes.size();
    slot.isFast = isFast;
    slot.socket = 0;

    if (!dataBytes.isEmpty()) {
        memcpy(slot.payload.data, dataBytes.constData(), dataBytes.size());
    }
    // 确保以 Null 结尾
    if (slot.size < MAX_PACKET_SIZE) {
        slot.payload.data[slot.size] = '\0';
    }

    // 6. 原子提交索引
    InterlockedExchange(pWriteIdx, next_write);

    // 7. 触发内核对象信号，通知 DLL
    if (ipc.m_hIpcEvent) {
        SetEvent(ipc.m_hIpcEvent);
    }

    DebugHelper::recordTreeLog(logFile, QString("✅ IPC [%1] 转发成功").arg(isFast ? "FAST" : "SLOW"), 1, true);
    return true;
}

bool NetworkManager::sendIpcBufferMessage(IpcMessageType type, const void *pData, size_t dataSize)
{
    const QString &logFile = "send_ipc_buffer_2";
    IpcManager &ipc = IpcManager::instance();

    // 1. 检查共享内存指针
    if (!ipc.m_pSharedData) {
        static qint64 lastIpcWarn = 0;
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastIpcWarn > 2000) {
            qWarning().noquote() << "❌ [IPC 严重错误] 共享内存尚未初始化";
            lastIpcWarn = now;
        }
        DebugHelper::recordTreeLog(logFile, QString("❌ IPC 失败: 共享内存未就绪"), 1, true);
        return false;
    }

    // 2. 检查输入数据指针
    if (!pData) {
        qWarning().noquote() << "❌ [IPC 错误] pData 为空 | 类型:" << type;
        return false;
    }

    auto *sharedData = ipc.m_pSharedData;

    // 3. 长度检查
    if (dataSize > MAX_PACKET_SIZE) {
        qWarning() << "❌ [IPC Buffer] 数据过长 (" << dataSize << " bytes)";
        DebugHelper::recordTreeLog(logFile, QString("❌ IPC 失败: 载荷过长 (%1)").arg(dataSize), 1, true);
        return false;
    }

    // 4. 优先级路由逻辑
    bool isFast = isFastMessage(type);
    volatile LONG *pWriteIdx = isFast ? &sharedData->fast_write_index_l2d : &sharedData->slow_write_index_l2d;
    volatile LONG *pReadIdx  = isFast ? &sharedData->fast_read_index_l2d  : &sharedData->slow_read_index_l2d;
    MessageSlot   *pBuffer   = isFast ? sharedData->fast_buffer_l2d       : sharedData->slow_buffer_l2d;

    // 5. 负载监控逻辑
    LONG r = *pReadIdx;
    LONG w = *pWriteIdx;

    int currentUsage = (w >= r) ? (w - r) : (BUFFER_SIZE - r + w);
    int usagePercent = (currentUsage * 100) / BUFFER_SIZE;

    if (usagePercent >= 80) {
        qWarning() << QString("⚠ [IPC %1 Load] 负载高! %2/%3 (%4%) | 类型: %5")
                          .arg(isFast ? "FAST" : "SLOW").arg(currentUsage).arg(BUFFER_SIZE).arg(usagePercent).arg(type);
    }

    // 6. 写入位置计算与满位检查
    LONG next_write = (w + 1) % BUFFER_SIZE;
    if (next_write == r) {
        qWarning() << QString("🔥 [IPC %1 Overload] 缓冲区满！丢弃类型: %2").arg(isFast ? "FAST" : "SLOW").arg(type);
        DebugHelper::recordTreeLog(logFile, "❌ IPC 失败: 缓冲区溢出", 1, true);
        return false;
    }

    // 7. 获取槽位并填充
    MessageSlot &slot = pBuffer[w];
    slot.type = type;
    slot.size = static_cast<DWORD>(dataSize);
    slot.isFast = isFast;
    slot.socket = 0;

    memcpy(slot.payload.data, pData, dataSize);

    // 如果空间允许，在末尾补一个零
    if (dataSize < MAX_PACKET_SIZE) {
        slot.payload.data[dataSize] = '\0';
    }

    // 8. 原子提交索引
    InterlockedExchange(pWriteIdx, next_write);

    // 9. 触发内核事件通知 DLL
    if (ipc.m_hIpcEvent) {
        SetEvent(ipc.m_hIpcEvent);
    }

    DebugHelper::recordTreeLog(logFile, QString("✅ IPC [%1] 转发成功| 长度: %2").arg(isFast ? "FAST" : "SLOW").arg(dataSize), 1, true);

    return true;
}

void NetworkManager::startNetworkTimers()
{
    startHeartbeat();
    m_networkCheckTimer->start();
}

void NetworkManager::stopListening()
{
    if (thread() != QThread::currentThread()) {
        qDebug() << "⚠ 跨线程请求清理，执行同步阻塞调用...";
        QMetaObject::invokeMethod(this, "stopListening", Qt::QueuedConnection);
        return;
    }

    if (!m_isListening.loadRelaxed()) return;

    qDebug() << "🧊 开始断开所有网络监听服务...";

    m_isListening.storeRelaxed(false);
    m_heartbeatRunning.storeRelaxed(false);

    cleanupTimers();

    safeCleanup();
    qDebug() << "✅ 网络层清理彻底完成。";
}

void NetworkManager::cleanupTimers()
{
    if (m_udpHeartbeatTimer) m_udpHeartbeatTimer->stop();
    if (m_tcpHeartbeatTimer) m_tcpHeartbeatTimer->stop();
    if (m_networkCheckTimer) m_networkCheckTimer->stop();
    if (m_registrationTimer) m_registrationTimer->stop();
}

void NetworkManager::safeCleanup()
{
    qDebug() << "🧹 执行网络资源深度清理...";
    cleanupExistingPublicSocket();
    cleanupExistingGameSocket();
    cleanupExistingTcpSocket();
    resetConnectionState();
}

void NetworkManager::cleanupExistingTcpSocket()
{
    if (m_tcpSocket) {
        qDebug() << "🔌 正在关闭 TCP 控制通道...";
        m_tcpSocket->disconnect();
        if (m_tcpSocket->isOpen()) {
            m_tcpSocket->abort();
        }
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }
}
void NetworkManager::resetConnectionState()
{
    m_udpLastPongTime.store(0);
    m_udpLastPingTime.store(0);
    m_udpConsecutiveFailures = 0;
    m_sessionId = 0;
    m_localSeq = 0;
    m_registrationState = Unregistered;
}

void NetworkManager::gracefulExit()
{
    qDebug() << "🛑 收到程序退出信号...";
    if (m_registrationState == Registered) {
        unregisterToServer();
    }
    stopListening();
}

// ==================== 二进制协议核心发送逻辑 ====================

QByteArray NetworkManager::preparePacket(PacketType type, const void *payloadData, quint16 payloadSize)
{
    // --- 1. 开始构建 ---
    qDebug() << "┌── [构建数据包] 开始...";
    qDebug() << "│   ├── 类型(Type):" << (int)type << "| 负载长度:" << payloadSize << "字节";

    int headerSize = sizeof(PacketHeader);
    QByteArray buffer(headerSize + payloadSize, '\0');

    PacketHeader *header = reinterpret_cast<PacketHeader*>(buffer.data());
    header->magic        = PROTOCOL_MAGIC;
    header->version      = PROTOCOL_VERSION;
    header->command      = static_cast<quint8>(type);
    header->sessionId    = m_sessionId;
    header->seq          = ++m_localSeq;
    header->payloadLen   = payloadSize;

    // 清空校验位
    header->checksum = 0;
    memset(header->signature, 0, 16);

    // --- 2. 写入包头详情 ---
    qDebug() << "│   ├── [包头字段]";
    qDebug() << "│   │   ├── 魔数(Magic):  " << QString("0x%1").arg(header->magic, 0, 16).toUpper();
    qDebug() << "│   │   ├── 序列号(Seq):  " << header->seq;
    qDebug() << "│   │   └── 会话ID(SID):  " << header->sessionId;

    // --- 3. 填充负载 ---
    if (payloadSize > 0 && payloadData != nullptr) {
        memcpy(buffer.data() + headerSize, payloadData, payloadSize);
        if (payloadSize <= 32) {
            qDebug() << "│   ├── [负载内容] 十六进制:" << QByteArray((const char*)payloadData, payloadSize).toHex(' ').toUpper();
        } else {
            qDebug() << "│   ├── [负载内容] 长度较大，跳过打印详情";
        }
    } else {
        qDebug() << "│   ├── [负载内容] 无 (空包)";
    }

    // --- 4. 签名计算 ---
    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(buffer);
    QString secret = SettingsManager::instance().appSecret();
    hasher.addData(secret.toUtf8());
    QByteArray fullHash = hasher.result();

    memcpy(header->signature, fullHash.constData(), 16);

    qDebug() << "│   ├── [安全签名]";
    qDebug() << "│   │   ├── 通信密钥状态:" << (!secret.isEmpty() ? "已配置" : "未配置 (警告!)");
    qDebug() << "│   │   └── 签名摘要(16B):" << QByteArray((const char*)header->signature, 16).toHex().toUpper();

    // --- 5. CRC 校验 ---
    header->checksum = calculateCRC16(buffer);
    qDebug() << "│   └── [CRC校验] 结果:" << QString("0x%1").arg(header->checksum, 4, 16, QChar('0')).toUpper();

    // --- 6. 结束 ---
    qDebug() << "└── [构建数据包] 成功。总长度:" << buffer.size() << "字节";

    return buffer;
}

quint64 NetworkManager::sendTcpPacket(PacketType type, const void *payloadData, quint16 payloadSize)
{
    return sendTcpRaw(preparePacket(type, payloadData, payloadSize));
}

quint64 NetworkManager::sendUdpPacket(PacketType type, const void *payloadData, quint16 payloadSize)
{
    return sendUdpRaw(preparePacket(type, payloadData, payloadSize), m_serverAddress, m_serverPort);
}

quint64 NetworkManager::sendTcpRaw(const QByteArray &data)
{
    qDebug().noquote() << "📤 [TCP 传输层]";

    // 1. 检查 Socket
    if (!m_tcpSocket) {
        qDebug().noquote() << "   └─ ❌ 错误: TCP Socket 未初始化";
        return -1;
    }

    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug().noquote() << "   └─ ❌ 错误: TCP 未连接 (State: " << m_tcpSocket->state() << ")";
        return -1;
    }

    // 2. 执行发送
    quint64 written = m_tcpSocket->write(data);

    // 3. 日志
    if (written == -1) {
        qDebug().noquote() << QString("   └─ ❌ 失败: %1").arg(m_tcpSocket->errorString());
        return -1;
    }

    qDebug().noquote() << QString("   ├─ 🎯 目标: 服务端控制通道");
    qDebug().noquote() << QString("   └─ ✅ 状态: 发送成功 (%1 字节)").arg(written);

    return written;
}

quint64 NetworkManager::sendUdpRaw(const QByteArray &data, const QHostAddress &address, quint16 port)
{
    qDebug().noquote() << "📤 [UDP 传输层]";

    // 1. 检查 Socket
    if (!ensurePublicSocketCreated()) {
        qDebug().noquote() << "   ├─ ❌ 错误: Socket 创建失败";
        qDebug().noquote() << "   └─ 🚫 结果: 放弃发送";
        return -1;
    }

    // 2. 检查地址
    if (address.isNull() || port == 0) {
        qDebug().noquote() << QString("   ├─ ❌ 错误: 无效目标 %1:%2").arg(address.toString()).arg(port);
        qDebug().noquote() << "   └─ 🚫 结果: 放弃发送";
        return -1;
    }

    // 3. 执行发送
    quint64 written = m_publicSocket->writeDatagram(data, address, port);
    QString targetStr = QString("%1:%2").arg(address.toString()).arg(port);

    // 4. 处理结果日志
    if (written == -1) {
        qDebug().noquote() << QString("   ├─ 🎯 目标: %1").arg(targetStr);
        qDebug().noquote() << QString("   └─ ❌ 失败: %1").arg(m_publicSocket->errorString());
        return -1;
    }

    // 更新统计
    if (written > 0) {
        m_totalBytesSent += written;
        m_totalPacketsSent++;
    }

    qDebug().noquote() << QString("   ├─ 🎯 目标: %1").arg(targetStr);
    qDebug().noquote() << QString("   ├─ 📡 载荷: %1 字节").arg(written);
    qDebug().noquote() << QString("   └─ ✅ 状态: 发送成功 (累计包数: %1)").arg(m_totalPacketsSent);

    return written;
}

// ==================== 业务指令实现 ====================

void NetworkManager::registerToServer()
{
    // 0. 线程安全检查
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "registerToServer", Qt::QueuedConnection);
        return;
    }

    qDebug().noquote() << "┌── [用户注册预检] 发起流程 (TCP 模式)...";

    // 1. 身份与凭证校验
    const QString &userName = "*Translator";
    const QString &clientId = SettingsManager::instance().clientId();
    const QString &hardwareId = SettingsManager::instance().hardwareId();
    if (userName.isEmpty() || hardwareId.isEmpty()) {
        qDebug().noquote() << "├── ❌ 身份缺失，终止流程";
        return;
    }

    // 2. 状态校验
    if (m_registrationState == Registering || m_registrationState == Registered) {
        qDebug().noquote() << "├── ⚠  拦截: 当前状态为 " << getRegistrationStateString(m_registrationState);
        return;
    }

    // 3. TCP 连接预检
    if (!m_tcpSocket) {
        qDebug().noquote() << "   └─ ❌ 错误: TCP Socket 未初始化";
        return;
    }

    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug().noquote() << "├── 🔌 TCP 未连接，正在尝试建立控制连接...";
        m_registrationState = Registering;
        connectToTcpServer();
        return;
    }

    // 4. UDP 端口预备
    ensurePublicSocketCreated();
    LocalAddressInfo localInfo = getLocalAddressInfo();

    // 5. 信息汇总打印
    qDebug().noquote() << QString("├── 凭证: 用户: %1 | HWID: %2").arg(userName, hardwareId);
    qDebug().noquote() << QString("├── UDP 映射: %1:%2")
                              .arg(m_detectedPublicIp.isEmpty() ? "未探测" : m_detectedPublicIp)
                              .arg(m_detectedPublicPort);

    // 6. 构造并执行 TCP 发送
    m_registrationState = Registering;
    m_localSeq = 0;
    m_sessionId = 0;

    CSRegisterPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    qstrncpy(pkt.userName, userName.toUtf8().constData(), sizeof(pkt.userName));
    qstrncpy(pkt.clientId, clientId.toUtf8().constData(), sizeof(pkt.clientId));
    qstrncpy(pkt.hardwareId, hardwareId.toUtf8().constData(), sizeof(pkt.hardwareId));
    qstrncpy(pkt.localIp,  localInfo.ip.toUtf8().constData(), sizeof(pkt.localIp));
    qstrncpy(pkt.publicIp, m_detectedPublicIp.toUtf8().constData(), sizeof(pkt.publicIp));
    pkt.localPort  = localInfo.port;
    pkt.publicPort = m_detectedPublicPort;

    quint64 bytesSent = sendTcpPacket(C_S_REGISTER, &pkt, sizeof(pkt));

    if (bytesSent > 0) {
        qDebug().noquote() << QString("└── ✅ 发送成功: 注册请求已通过 TCP 发出");
        m_registrationTimer->start(15000); // TCP 虽然可靠，但仍需防止业务层不回包
    } else {
        qDebug().noquote() << "└── ❌ 发送失败: TCP 链路异常";
        m_registrationState = Failed;
    }
}

void NetworkManager::unregisterToServer()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "unregisterToServer", Qt::QueuedConnection);
        return;
    }

    if (m_registrationState == Unregistered) return;

    quint32 oldSession = m_sessionId;

    qDebug().noquote() << "👋 [用户注销流程]";

    // 1. 优先通过 TCP 发送注销指令
    if (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug().noquote() << "   ├─ 📤 执行动作: 通过 TCP 发送 C_S_UNREGISTER";
        sendTcpPacket(C_S_UNREGISTER, nullptr, 0);
    } else {
        qDebug().noquote() << "   ├─ 📤 执行动作: TCP 已断开，尝试 UDP 兜底注销";
        sendUdpPacket(C_S_UNREGISTER);
    }

    // 2. 停止所有定时器
    stopHeartbeat();
    if (m_registrationTimer->isActive()) m_registrationTimer->stop();
    if (m_tcpHeartbeatTimer->isActive()) m_tcpHeartbeatTimer->stop();

    // 3. 重置本地状态
    m_registrationState = Unregistered;
    m_sessionId = 0;
    m_localSeq = 0;

    // 4. 重置计数器
    m_udpConsecutiveFailures.store(0);
    m_udpLastPongTime.store(0);
    m_tcpLastPongTime.store(0);

    // 5. 主动关闭物理连接
    if (m_tcpSocket) {
        qDebug().noquote() << "   ├─ 🔌 物理断开: 主动关闭 TCP 控制链路";
        m_tcpSocket->disconnectFromHost();
    }

    qDebug().noquote() << QString("   ├─ ♻ 会话清理: Session(%1) -> 0").arg(oldSession);
    qDebug().noquote() << "   └─ ✅ 最终状态: 本地已离线 (Unregistered)";

    emit unregistrationStatus(true);
    emit udpConnectionStatusChanged(Unregistered);
}

bool NetworkManager::pingToUDPServer()
{
    m_udpLastPingTime.store(QDateTime::currentMSecsSinceEpoch());
    return sendUdpPacket(C_S_PING) > 0;
}

void NetworkManager::sendCommandMessage(const QString &userName, const QString &command, const QString &text)
{
    // 线程重定向
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendCommandMessage", Qt::QueuedConnection,
                                  Q_ARG(QString, userName),
                                  Q_ARG(QString, command),
                                  Q_ARG(QString, text));
        return;
    }

    // 状态检查
    if (m_registrationState != Registered) {
        qDebug() << "❌ 未注册，无法发送Bot指令";
        emit unregisteredStateDetected();
        return;
    }

    // TCP 连接检查
    if (!m_tcpSocket) {
        qDebug().noquote() << "   └─ ❌ 错误: TCP Socket 未初始化";
        return;
    }
    if (m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "❌ TCP 未连接，无法发送指令";
        return;
    }

    const QString &clientId = SettingsManager::instance().clientId();

    CSCommandPacket pkt;
    memset(&pkt, 0, sizeof(pkt));

    qstrncpy(pkt.clientId, clientId.toUtf8().constData(), sizeof(pkt.clientId));
    qstrncpy(pkt.userName, userName.toUtf8().constData(), sizeof(pkt.userName));
    qstrncpy(pkt.command, command.toUtf8().constData(), sizeof(pkt.command));
    qstrncpy(pkt.text, text.toUtf8().constData(), sizeof(pkt.text));

    qDebug() << "📤 发送指令 (C_S_COMMAND via TCP):" << command << " 内容:" << text;

    sendTcpPacket(PacketType::C_S_COMMAND, &pkt, sizeof(pkt));
}

void NetworkManager::sendTranslatedMessage(quint32 pid, quint32 flag, quint32 extraScope, const QString &translatedMessage)
{
    // 1. 线程重定向
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendTranslatedMessage", Qt::QueuedConnection,
                                  Q_ARG(quint32, pid),
                                  Q_ARG(quint32, flag),
                                  Q_ARG(quint32, extraScope),
                                  Q_ARG(QString, translatedMessage));
        return;
    }

    DebugHelper::recordTreeLog("chat_translate", "┌─ 📤 [TCP] 准备向服务器同步译文", 0);

    // 2. 状态检查
    if (m_registrationState != Registered) {
        DebugHelper::recordTreeLog("chat_translate", "└─ ❌ 发送失败: 客户端未注册到服务器", 1, true);
        qDebug() << "❌ 未注册，无法发送翻译同步包";
        emit unregisteredStateDetected();
        return;
    }

    // 3. 基础参数校验
    if (translatedMessage.isEmpty()) {
        DebugHelper::recordTreeLog("chat_translate", "└─ ⚠ 发送中止: 译文内容为空", 1, true);
        return;
    }

    // 4. 构建数据包
    CSTranslatedMessagePacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.pid = pid;
    pkt.flag = flag;
    pkt.extraScope = extraScope;

    QByteArray ba = translatedMessage.toUtf8();
    qstrncpy(pkt.message, ba.constData(), sizeof(pkt.message));

    DebugHelper::recordTreeLog("chat_translate",
                               QString("├─ 📦 封包详情: 指令=0x25 | PID=%1 | 长度=%2字节").arg(pid).arg(ba.size()), 1);

    // 5. 执行 TCP 发送
    sendTcpPacket(PacketType::C_S_TRANSLATED_MESSAGE, &pkt, sizeof(pkt));

    DebugHelper::recordTreeLog("chat_translate", "└─ ✅ 译文数据包已物理写入网络缓冲区", 1, true);

    qDebug().noquote() << "📤 [TCP] 已向服务器上报翻译内容";
    qDebug().noquote() << "   ├─ 来源 PID:" << pid;
    qDebug().noquote() << "   └─ 内容:" << translatedMessage;
}

// ==================== 接收与处理 ====================

void NetworkManager::onTcpConnected()
{
    qDebug() << "✅ [TCP] 控制通道连接成功！本地端口:" << m_tcpSocket->localPort();
    m_tcpLastPongTime.store(QDateTime::currentMSecsSinceEpoch());
    m_tcpConsecutiveFailures.store(0);
    if (m_registrationState == Unregistered || m_registrationState == Registering) {
        m_registrationState = Unregistered;
        registerToServer();
    }

    if (m_sessionId != 0) {
        qDebug() << "📤 [TCP] 断线重连，发送身份绑定包 (Session:" << m_sessionId << ")";
        sendTcpPacket(PacketType::C_S_PING, nullptr, 0);
    }

    m_tcpHeartbeatTimer->start();
}

void NetworkManager::onTcpDisconnected()
{
    qDebug() << "❌ [TCP] 连接断开";
    m_tcpHeartbeatTimer->stop();
    emit networkDisconnected();
    QTimer::singleShot(3000, this, &NetworkManager::connectToTcpServer);
}

void NetworkManager::onTcpError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "❌ [TCP] 错误:" << m_tcpSocket->errorString();
}

void NetworkManager::onTcpReadyRead()
{
    while (m_tcpSocket->bytesAvailable() > 0) {

        // 1. 检查头部是否完整
        if (m_tcpSocket->bytesAvailable() < (qint64)sizeof(PacketHeader)) return;

        // 2. 预读头部，获取 payload 长度
        PacketHeader header;
        m_tcpSocket->peek(reinterpret_cast<char*>(&header), sizeof(PacketHeader));

        if (header.payloadLen > MAX_PACKET_SIZE) {
            qWarning() << "❌ [TCP] 收到非法超大包 (Len:" << header.payloadLen << ")，断开连接防攻击";
            m_tcpSocket->disconnectFromHost();
            return;
        }

        // 3. 校验魔数
        if (header.magic != PROTOCOL_MAGIC) {
            qWarning() << "❌ [TCP] 协议头错误，断开连接";
            m_tcpSocket->disconnectFromHost();
            return;
        }

        // 4. 等待完整包
        qint64 totalPacketSize = sizeof(PacketHeader) + header.payloadLen;
        if (m_tcpSocket->bytesAvailable() < totalPacketSize) {
            return; // 等待更多数据
        }

        // 5. 读取
        QByteArray data = m_tcpSocket->read(totalPacketSize);

        // 6. TCP 安全签名校验
        // A. 提取收到的签名并清零原始缓冲区中的签名位
        PacketHeader *pRawHeader = reinterpret_cast<PacketHeader*>(data.data());
        PacketType type = static_cast<PacketType>(header.command);
        if (type == S_C_PONG && pRawHeader->sessionId == 0) {
            m_tcpLastPongTime.store(QDateTime::currentMSecsSinceEpoch());
            m_tcpConsecutiveFailures.store(0);
            qDebug().noquote() << "📥 [TCP] 收到 Pong (心跳回包)";
            return;
        }

        char receivedSign[16];
        memcpy(receivedSign, pRawHeader->signature, 16);
        memset(pRawHeader->signature, 0, 16);

        // B. 使用客户端 Secret 重新计算
        QByteArray secret = SettingsManager::instance().appSecret();
        QByteArray signSource = data + secret;
        QByteArray expectedHash = QCryptographicHash::hash(signSource, QCryptographicHash::Sha256);

        // C. 比对签名
        if (memcmp(receivedSign, expectedHash.constData(), 16) != 0) {
            qWarning() << "⚠ [TCP安全拦截] 服务器签名校验失败，断开连接！";
            m_tcpSocket->disconnectFromHost();
            return;
        }

        // 7. 执行 CRC 校验
        quint16 recvChecksum = pRawHeader->checksum;
        pRawHeader->checksum = 0;
        if (calculateCRC16(data) != recvChecksum) {
            qWarning() << "❌ [TCP] CRC校验失败";
            continue;
        }

        const char *payload = data.constData() + sizeof(PacketHeader);

        // 8. 处理指令 (S_C_COMMAND, S_C_PONG 等)
        switch (type) {
        case S_C_REGISTER: {
            if (header.payloadLen >= sizeof(SCRegisterPacket)) {
                handleRegisterResponse(reinterpret_cast<const SCRegisterPacket*>(payload));
            } else {
                qWarning() << "❌ [TCP] 收到注册响应包，但长度不足";
            }
            break;
        }

        default:
            qDebug() << "⚠ [TCP] 未知包类型:" << header.command;
            break;
        }
    }
}

void NetworkManager::onTcpStateChanged(QAbstractSocket::SocketState socketState)
{
    QString stateStr;
    bool isConnected = false;

    switch (socketState) {
    case QAbstractSocket::UnconnectedState:
        stateStr = "Unconnected (已断开/未连接)";
        isConnected = false;
        break;
    case QAbstractSocket::HostLookupState:
        stateStr = "HostLookup (正在解析服务器地址...)";
        break;
    case QAbstractSocket::ConnectingState:
        stateStr = "Connecting (正在建立 TCP 握手...)";
        break;
    case QAbstractSocket::ConnectedState:
        stateStr = "Connected (✅ 物理链路已就绪)";
        isConnected = true;
        break;
    case QAbstractSocket::BoundState:
        stateStr = "Bound (本地端口已绑定)";
        break;
    case QAbstractSocket::ClosingState:
        stateStr = "Closing (正在关闭连接...)";
        isConnected = false;
        break;
    default:
        stateStr = QString("Unknown (%1)").arg(static_cast<int>(socketState));
        break;
    }

    qDebug().noquote() << "🔌 [TCP 状态监测]";
    qDebug().noquote() << "   └─ 🔄 变更 -> " << stateStr;

    emit tcpStatusChanged(isConnected, stateStr);

    if (socketState == QAbstractSocket::UnconnectedState) {
        if (m_registrationState == Registered) {
            qDebug().noquote() << "   ⚠ 检测到链路断开，业务状态回滚至 Unregistered";
        }
    }
}

void NetworkManager::onPublicSocketReadyRead()
{
    if (!m_publicSocket) return;

    while (m_publicSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_publicSocket->receiveDatagram();
        m_totalBytesReceived += datagram.data().size();
        m_totalPacketsReceived++;
        handleIncomingDatagram(datagram);
    }
}

void NetworkManager::onTranslationFinished(quint32 pid, quint32 flag, quint32 extraScope, QString originalMessage, QString translatedMessage)
{
    DebugHelper::recordTreeLog("chat_translate", "┌─ 🔔 [NetworkManager] 接收到翻译任务结果", 0);

    QString finalMessage = translatedMessage;
    bool isFallback = false;

    if (finalMessage.isEmpty()) {
        finalMessage = originalMessage;
        isFallback = true;
        DebugHelper::recordTreeLog("chat_translate", "├─ ⚠ 翻译失败/结果为空，执行原文回退策略", 1);
    }

    if (!isFallback && originalMessage == translatedMessage) {
        DebugHelper::recordTreeLog("chat_translate", "├─ ℹ️ 翻译结果与原文完全一致，无需转换)", 1);
    }

    DebugHelper::recordTreeLog("chat_translate",
                               QString("├─ 👤 上下文: PID=%1 | Flag=0x%2 | Scope=0x%3")
                                   .arg(pid).arg(QString::number(flag, 16).toUpper(), QString::number(extraScope, 16).toUpper()), 1);

    DebugHelper::recordTreeLog("chat_translate",
                               QString("├─ 📝 内容: \"%1\" -> \"%2\"")
                                   .arg(originalMessage, finalMessage), 1);

    TranslatedResultPayload payload;
    memset(&payload, 0, sizeof(payload));
    payload.pid = pid;
    payload.flag = flag;
    payload.extraScope = extraScope;

    QByteArray srcBa = originalMessage.toUtf8();
    QByteArray dstBa = finalMessage.toUtf8();

    qstrncpy(payload.originalMessage, srcBa.data(), sizeof(payload.originalMessage));
    qstrncpy(payload.translatedMessage, dstBa.data(), sizeof(payload.translatedMessage));

    sendIpcBufferMessage(MSG_TYPE_TRANSLATE_RESPONSE, &payload, sizeof(payload));

    DebugHelper::recordTreeLog("chat_translate",
                               QString("└─ 🚀 结果已发回 DLL (状态: %1)")
                                   .arg(isFallback ? "回退原文" : "翻译成功"), 0, true);
}

void NetworkManager::handleIncomingDatagram(const QNetworkDatagram &datagram)
{
    QByteArray data = datagram.data();
    QHostAddress sender = datagram.senderAddress();
    quint16 port = datagram.senderPort();

    // 1. 过滤非服务器流量
    if (sender != m_serverAddress || port != m_serverPort) {
        // P2P 逻辑或其他来源
        return;
    }

    // 2. 长度检查
    if (data.size() < (int)sizeof(PacketHeader)) return;

    // 3. 映射 Header
    PacketHeader *header = reinterpret_cast<PacketHeader*>(data.data());

    // 4. 魔数与版本检查
    if (header->magic != PROTOCOL_MAGIC || header->version != PROTOCOL_VERSION) {
        qDebug() << "🗑 协议不匹配，丢弃数据包";
        return;
    }

    // 安全签名校验 (HMAC)
    char receivedSign[16];
    memcpy(receivedSign, header->signature, 16); // 提取服务器发来的签名
    memset(header->signature, 0, 16);            // 计算前清零

    QByteArray secret = SettingsManager::instance().appSecret();
    QByteArray signSource = data + secret;
    QByteArray expectedHash = QCryptographicHash::hash(signSource, QCryptographicHash::Sha256);

    if (memcmp(receivedSign, expectedHash.constData(), 16) != 0) {
        qDebug() << "🚫 [安全拦截] 服务器响应签名校验失败，可能是伪造服务器！";
        return;
    }

    // 5. 长度一致性检查
    if (data.size() != sizeof(PacketHeader) + header->payloadLen) {
        qDebug() << "🗑 包长度错误，丢弃";
        return;
    }

    // 6. CRC 校验
    quint16 recvChecksum = header->checksum;
    header->checksum = 0; // 清零以计算
    if (calculateCRC16(data) != recvChecksum) {
        qDebug() << "❌ CRC校验失败，丢弃";
        return;
    }

    // 7. 分发处理
    char *payload = data.data() + sizeof(PacketHeader);
    PacketType cmd = static_cast<PacketType>(header->command);

    switch (cmd) {
    case S_C_REGISTER: {
        if (header->payloadLen >= sizeof(SCRegisterPacket))
            handleRegisterResponse(reinterpret_cast<SCRegisterPacket*>(payload));
        break;
    }

    case S_C_PONG:
    case C_S_HEARTBEAT: {
        if (header->payloadLen >= sizeof(SCPongPacket)) {
            handlePongResponse(reinterpret_cast<SCPongPacket*>(payload));
        } else {
            SCPongPacket packet;
            packet.status = 0;
            handlePongResponse(&packet);
        }
        break;
    }

    default:
        qDebug() << "❓ 未知指令:" << (int)cmd;
        break;
    }
}

// ==================== 响应处理具体实现 ====================

void NetworkManager::handleRegisterResponse(const SCRegisterPacket *packet)
{
    if (packet->status == 2) { // 假设 2 是成功状态
        m_sessionId = packet->sessionId;
        m_registrationState = Registered;
        m_registrationTimer->stop();

        m_udpLastPongTime.store(QDateTime::currentMSecsSinceEpoch());
        m_udpConsecutiveFailures.store(0);

        qDebug().noquote() << "┌── [注册成功] 身份已确认";
        qDebug().noquote() << "├── SessionID:" << m_sessionId;
        qDebug().noquote() << "└── 📡 发起 UDP 端口绑定探测...";
        sendUdpPacket(C_S_HEARTBEAT);

        emit registrationStatus(true, "注册成功");
        emit udpConnectionStatusChanged(Registered);

        startHeartbeat();
    } else {
        m_registrationState = Failed;
        qDebug() << "❌ 注册失败: 服务器拒绝";
        emit registrationStatus(false, "服务器拒绝");
    }
}

void NetworkManager::handlePongResponse(const SCPongPacket *packet)
{
    // 1. 检查服务端返回的状态
    if (packet->status == Unregistered) {
        if (m_registrationState == Registered) {
            qDebug() << "🚨 [状态同步] 服务端报告 Session 失效，立即触发重连...";
            // 强制重置本地状态
            m_registrationState = Unregistered;
            m_sessionId = 0;
            m_udpConsecutiveFailures.store(0);
            m_udpLastPongTime.store(0);
        }
        // 立即发起注册
        emit unregisteredStateDetected();
        return; // 结束，不要更新心跳时间
    }

    // 2. 状态正常 (status == 2)，更新心跳时间
    quint64 now = QDateTime::currentMSecsSinceEpoch();
    int oldFailures = m_udpConsecutiveFailures.load();

    m_udpLastPongTime.store(now);
    m_tcpLastPongTime.store(now);
    m_udpConsecutiveFailures.store(0);
    m_tcpConsecutiveFailures.store(0);

    if (oldFailures > 0) {
        qDebug().noquote() << QString("💚 [网络恢复] 收到 PONG (之前失败次数: %1)").arg(oldFailures);
    }

    emit udpConnectionStatusChanged(RegistrationState(packet->status));
}

void NetworkManager::handleChatCommand(const QString &fullText, const QString &userName)
{
    // 0. 线程重定向
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "handleChatCommand", Qt::QueuedConnection,
                                  Q_ARG(QString, fullText),
                                  Q_ARG(QString, userName));
        return;
    }

    qDebug() << "🧵 [NetworkThread] 正在处理指令:" << fullText;

    // 1. 基础分割与提取
    QStringList args = fullText.split(' ', Qt::SkipEmptyParts);
    if (args.isEmpty()) return;

    QString cmd = args.value(0).trimmed().toLower();
    QString param = fullText.section(' ', 1).trimmed();
    QString finalCmd = cmd;

    if (cmd == "/bot") {
        // 1. 转换
        ServerName serverName = SettingsManager::stringToServerName(param);

        // 2. 判断是否合法
        if (serverName == UNKNOWN) {
            qDebug().noquote() << "└── ❌ 错误: 无效节点名 '" << param << "'. 可用: CN1, US1";
            emit botChanged(serverName);
            return;
        }

        // 3. 只有合法才执行切换
        setupServer(serverName);

        qDebug().noquote() << "└── ✅ 节点已切换至:" << SettingsManager::serverNameToString(serverName);
        emit botChanged(serverName);
        return;
    }

    // 2. 统一执行发送
    sendCommandMessage(userName, cmd, param);
}

// ==================== 定时器与心跳 ====================

void NetworkManager::startHeartbeat()
{
    if (!m_udpHeartbeatTimer->isActive()) {
        m_udpHeartbeatTimer->start();
        qDebug() << "⏰ [UDP]  心跳定时器已启动 (Interval: 10s)";
    }
    m_heartbeatRunning.storeRelaxed(true);
}

void NetworkManager::stopHeartbeat()
{
    if (m_udpHeartbeatTimer) m_udpHeartbeatTimer->stop();
}

void NetworkManager::onTcpHeartbeatTimeout()
{
    const QString logFile = "network_connection_state";

    // 1. 基础状态检查
    bool isConnected = (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState);

    if (!m_heartbeatRunning.loadRelaxed() || !isConnected) {
        static int tcpSkipCount = 0;
        if (++tcpSkipCount % 6 == 0) {
            qDebug().noquote() << "⏳ [TCP 心跳跳过] 当前状态不满足:";
            qDebug().noquote() << "   ├─ 开关状态: " << (m_heartbeatRunning.loadRelaxed() ? "✅ On" : "❌ Off");
            qDebug().noquote() << "   └─ 连接状态: " << (isConnected ? "✅ Connected" : "❌ Disconnected");
        }

        if (!isConnected) {
            DebugHelper::recordTreeLog(logFile, "⏳ [TCP心跳] 检测到物理断开", 0, false);
            DebugHelper::recordTreeLog(logFile, "执行动作: 尝试触发 connectToTcpServer", 1, true);
            connectToTcpServer();
        }

        return;
    }

    quint64 now = QDateTime::currentMSecsSinceEpoch();
    quint64 lastPong = m_tcpLastPongTime.load();

    // 2. 初始化检查 (首次运行)
    if (lastPong == 0) {
        m_tcpLastPongTime.store(now);
        qDebug().noquote() << "💓 [TCP 心跳监测] 初始化完成，基准时间已校准";
        DebugHelper::recordTreeLog(logFile, "💓 [TCP心跳] 初始化基准时间校准", 0, true);
        return;
    }

    // 3. 计算静默时间
    quint64 timeSinceLastPong = now - lastPong;

    // ==================== 场景 A: 严重超时断开 (> 60s) ====================
    if (timeSinceLastPong > 60000) {
        qDebug().noquote() << "🚨 [TCP 连接严重超时]";
        qDebug().noquote() << QString("   ├─ ⏱ 沉默时长: %1 ms (阈值: 60000 ms)").arg(timeSinceLastPong);
        qDebug().noquote() << QString("   ├─ 📅 最后响应: %1").arg(QDateTime::fromMSecsSinceEpoch(lastPong).toString("HH:mm:ss.zzz"));
        qDebug().noquote() << "   ├─ 🛑 执行动作: 强制断开 Socket 连接";
        qDebug().noquote() << "   └─ 🔄 后续操作: 触发断线重连逻辑";

        DebugHelper::recordTreeLog(logFile, "🚨 [TCP心跳] 链路严重超时 (>60s)", 0, false);
        DebugHelper::recordTreeLog(logFile, QString("静默时长: %1 ms").arg(timeSinceLastPong), 1, false);
        DebugHelper::recordTreeLog(logFile, "执行动作: 物理断开并重置状态", 1, true);

        // 重置状态
        m_tcpConsecutiveFailures.store(0);
        m_tcpLastPongTime.store(0); // 归零

        // TCP 特有操作：强制中断连接，触发 error 或 disconnected 信号进行重连
        if (m_tcpSocket) {
            m_tcpSocket->abort();
        }
        return;
    }

    // ==================== 场景 B: 网络波动警告 (> 15s) ====================
    if (timeSinceLastPong > 15000) {
        m_tcpConsecutiveFailures++;
        qDebug().noquote() << "⚠ [TCP 心跳监测 - 网络波动]";
        qDebug().noquote() << QString("   ├─ 🐌 当前延迟: %1 ms").arg(timeSinceLastPong);
        qDebug().noquote() << QString("   ├─ 📉 累计异常: %1 次 (疑似拥塞)").arg(m_tcpConsecutiveFailures);
        qDebug().noquote() << "   └─ 🚑 补救措施: 发送 C_S_HEARTBEAT 探测";

        DebugHelper::recordTreeLog(logFile, "⚠ [TCP心跳] 检测到网络波动/拥塞", 0, false);
        DebugHelper::recordTreeLog(logFile, QString("延迟: %1 ms | 异常计数: %2").arg(timeSinceLastPong).arg(m_tcpConsecutiveFailures.load()), 1, false);
        DebugHelper::recordTreeLog(logFile, "执行动作: 发送 C_S_HEARTBEAT 强行保活", 1, true);
    }
    // ==================== 场景 C: 健康状态 (<= 15s) ====================
    else {
        m_tcpConsecutiveFailures.store(0);
        qDebug().noquote() << "💓 [TCP 心跳监测 - 连接正常]";
        qDebug().noquote() << QString("   ├─ 📶 链路状态: 活跃 (间隔 < 15s)");
        qDebug().noquote() << QString("   └─ 📤 执行动作: 发送 C_S_HEARTBEAT 保活");
    }

    // 4. 发送 C_S_HEARTBEAT
    sendTcpPacket(PacketType::C_S_HEARTBEAT, nullptr, 0);
}

void NetworkManager::onUdpHeartbeatTimeout()
{
    const QString logFile = "network_connection_state";

    // 1. 基础状态检查
    if (!m_heartbeatRunning.loadRelaxed() || m_registrationState != Registered) {
        static int skipCount = 0;
        if (++skipCount % 6 == 0) {
            qDebug().noquote() << "⏳ [心跳跳过] 当前状态不满足:";
            qDebug().noquote() << "   ├─ 开关状态: " << (m_heartbeatRunning.loadRelaxed() ? "✅ On" : "❌ Off");
            qDebug().noquote() << "   └─ 注册状态: " << getRegistrationStateString(m_registrationState);
        }

        if(m_registrationState != Registered) {
            DebugHelper::recordTreeLog(logFile, "⏳ [UDP心跳] 当前未注册，准备重连...", 0, false);
            DebugHelper::recordTreeLog(logFile, QString("当前状态: %1").arg(getRegistrationStateString(m_registrationState)), 1, true);
            emit unregisteredStateDetected();
        }

        return;
    }

    quint64 now = QDateTime::currentMSecsSinceEpoch();
    quint64 lastPong = m_udpLastPongTime.load();

    // 2. 初始化检查 (首次运行)
    if (lastPong == 0) {
        m_udpLastPongTime.store(now);
        qDebug().noquote() << "💓 [心跳监测] 初始化完成，基准时间已校准";
        DebugHelper::recordTreeLog(logFile, "💓 [UDP心跳] 初始化基准时间校准", 0, true);
        return;
    }

    // 3. 计算静默时间
    quint64 timeSinceLastPong = now - lastPong;

    // ==================== 场景 A: 严重超时断开 (> 60s) ====================
    if (timeSinceLastPong > 60000) {
        qDebug().noquote() << "🚨 [连接严重超时]";
        qDebug().noquote() << QString("   ├─ ⏱ 沉默时长: %1 ms (阈值: 60000 ms)").arg(timeSinceLastPong);
        qDebug().noquote() << QString("   ├─ 📅 最后响应: %1").arg(QDateTime::fromMSecsSinceEpoch(lastPong).toString("HH:mm:ss.zzz"));
        qDebug().noquote() << "   ├─ 🛑 执行动作: 强制重置连接状态";
        qDebug().noquote() << "   └─ 🔄 后续操作: 触发断线重连 (Re-Register)";

        DebugHelper::recordTreeLog(logFile, "🚨 [UDP心跳] 链路严重超时 (>60s)", 0, false);
        DebugHelper::recordTreeLog(logFile, QString("静默时长: %1 ms").arg(timeSinceLastPong), 1, false);
        DebugHelper::recordTreeLog(logFile, "执行动作: 强制重置 Session 并触发 Re-Register", 1, true);

        // 重置状态
        m_registrationState = Unregistered;
        m_sessionId = 0;
        m_udpConsecutiveFailures = 0;
        m_udpLastPongTime.store(0); // 归零

        // 尝试重连
        emit unregisteredStateDetected();
        return; // 重连后直接返回，不再发送 Ping
    }

    // ==================== 场景 B: 网络波动警告 (> 15s) ====================
    if (timeSinceLastPong > 15000) {
        m_udpConsecutiveFailures++;

        qDebug().noquote() << "⚠ [心跳监测 - 网络波动]";
        qDebug().noquote() << QString("   ├─ 🐌 当前延迟: %1 ms").arg(timeSinceLastPong);
        qDebug().noquote() << QString("   ├─ 📉 累计异常: %1 次 (疑似丢包)").arg(m_udpConsecutiveFailures);
        qDebug().noquote() << "   └─ 🚑 补救措施: 发送 C_S_PING 探测";

        DebugHelper::recordTreeLog(logFile, "⚠ [UDP心跳] 检测到丢包/高延迟", 0, false);
        DebugHelper::recordTreeLog(logFile, QString("静默时长: %1 ms | 连续失败: %2").arg(timeSinceLastPong).arg(m_udpConsecutiveFailures.load()), 1, false);
        DebugHelper::recordTreeLog(logFile, "执行动作: 发送 C_S_PING 探测", 1, true);
    }
    // ==================== 场景 C: 健康状态 (<= 15s) ====================
    else {
        m_udpConsecutiveFailures = 0;

        qDebug().noquote() << "💓 [心跳监测 - 连接正常]";
        qDebug().noquote() << QString("   ├─ 📶 链路状态: 活跃 (间隔 < 15s)");
        qDebug().noquote() << QString("   └─ 📤 执行动作: 发送 C_S_PING 保活");
    }

    // 4. 发送 C_S_HEARTBEAT
    sendUdpPacket(PacketType::C_S_HEARTBEAT);
}

void NetworkManager::onRegistrationTimeout()
{
    if (m_registrationState == Registering) {
        qDebug() << "⏰ 注册超时";
        m_registrationState = Failed;
        emit registrationStatus(false, "超时");
    }
}

// ==================== 辅助函数 ====================

quint16 NetworkManager::calculateCRC16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;

    // 获取数据长度
    int len = data.size();

    // 获取无符号指针
    const unsigned char *p = reinterpret_cast<const unsigned char*>(data.constData());

    for (int i = 0; i < len; i++) {
        // 取出字节 (再次确保是无符号)
        unsigned char byte = p[i];

        // 如果这里 byte 是负数，位运算结果会完全不同！
        unsigned char x = (unsigned char)((crc >> 8) ^ byte);
        x ^= x >> 4;
        crc = (quint16)((crc << 8) ^ (quint16)(x << 12) ^ (quint16)(x << 5) ^ (quint16)x);
    }

    return crc;
}

QString NetworkManager::getCurrentServerAddress() const
{
    return m_serverAddress.toString();
}

QString NetworkManager::getRegistrationStateString(RegistrationState state) const
{
    switch (state) {
    case Unregistered:
        return QStringLiteral("🚫 未注册 (Unregistered)");
    case Registering:
        return QStringLiteral("⏳ 注册中 (Registering)");
    case Registered:
        return QStringLiteral("✅ 已注册 (Registered)");
    case Failed:
        return QStringLiteral("❌ 注册失败 (Failed)");
    default:
        return QStringLiteral("❓ 未知状态 (%1)").arg(static_cast<int>(state));
    }
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError error) {
    QString errStr = m_publicSocket ? m_publicSocket->errorString() : "Unknown Error";
    qDebug() << "❌ Socket Error:" << error << errStr;
    emit networkError(errStr);
}

void NetworkManager::onSocketStateChanged(QAbstractSocket::SocketState state) const
{
    QString stateStr;
    switch (state) {
    case QAbstractSocket::UnconnectedState: stateStr = "未连接"; break;
    case QAbstractSocket::HostLookupState: stateStr = "主机查找"; break;
    case QAbstractSocket::ConnectingState: stateStr = "连接中"; break;
    case QAbstractSocket::ConnectedState: stateStr = "已连接"; break;
    case QAbstractSocket::BoundState: stateStr = "已绑定"; break;
    case QAbstractSocket::ListeningState: stateStr = "监听中"; break;
    case QAbstractSocket::ClosingState: stateStr = "关闭中"; break;
    default: stateStr = "未知状态"; break;
    }

    qDebug() << "🔌 Socket状态改变:" << stateStr;
}

void NetworkManager::onUdpConnectionStatusChanged(RegistrationState registrationState)
{
    m_registrationState = registrationState;
    if(registrationState == Registered){
        connectToTcpServer();
    }
}

void NetworkManager::onUnregisteredStateDetected()
{
    qDebug().noquote() << "   └─ 检测到未注册，尝试注册到服务器...";
    registerToServer();
}

LocalAddressInfo NetworkManager::getLocalAddressInfo()
{
    LocalAddressInfo info;
    info.valid = false;

    // 使用原有的获取逻辑
    if (!ensurePublicSocketCreated()) return info;

    info.port = m_publicSocket->localPort();
    info.ip = getLocalAddress().toString(); // 调用 getLocalAddress() 遍历网卡

    if (info.ip != "127.0.0.1" && info.port > 0) info.valid = true;
    return info;
}

QHostAddress NetworkManager::getLocalAddress() const {
    // 遍历网卡寻找真实IP的逻辑
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : qAsConst(interfaces)) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            for (const auto &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol)
                    return entry.ip();
            }
        }
    }
    return QHostAddress::LocalHost;
}