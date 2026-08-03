#include "IpcManager.h"
#include "../helpers/DebugHelper.h"
#include "../managers/NetworkManager.h"
#include "../managers/SettingsManager.h"
#include "../managers/TranslateManager.h"
#include <QRegularExpression>

IpcManager &IpcManager::instance()
{
    static IpcManager instance;
    return instance;
}

IpcManager::IpcManager(QObject *parent) : QObject(parent)
{

}

IpcManager::~IpcManager() {
    cleanup();
}

void IpcManager::cleanup()
{
    qDebug().noquote() << "🔗 [IPC 服务解绑]";
    QString threadId = QString::number((quintptr)QThread::currentThreadId());
    qDebug().noquote() << QString("   ├─ 🧵 执行线程: %1").arg(threadId);

    // 清理共享内存
    bool memReleased = false;
    if (m_pSharedData) {
        UnmapViewOfFile(m_pSharedData);
        m_pSharedData = nullptr;
        memReleased = true;
    }

    if (m_hSharedMemory) {
        CloseHandle(m_hSharedMemory);
        m_hSharedMemory = NULL;
        memReleased = true;
    }

    if (memReleased) {
        qDebug().noquote() << "   ├─ ♻ 内存释放: SharedMemory 句柄已关闭";
    }

    if (m_hIpcEvent) {
        CloseHandle(m_hIpcEvent);
        m_hIpcEvent = NULL;
    }

    qDebug().noquote() << "   └─ ✅ 最终状态: IPC 服务已完全卸载";

    this->disconnect();

    qDebug() << "✅ [IpcManager] 服务已彻底停止";
}

QString IpcManager::initIpcManager()
{
    if (m_pSharedData) {
        return QString();
    }

    qDebug() << "🚀 [IpcManager] 正在初始化共享内存...";

    // 1. 创建共享内存
    m_hSharedMemory = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        SHARED_MEM_NAME
        );

    if (m_hSharedMemory == NULL) {
        QString err = QString("CreateFileMapping 失败，错误码: %1").arg(GetLastError());
        qCritical() << "❌ [IpcManager]" << err;
        return err;
    }

    bool alreadyExists = (GetLastError() == ERROR_ALREADY_EXISTS);

    // 2. 创建同步事件
    m_hIpcEvent = CreateEventW(NULL, FALSE, FALSE, IPC_EVENT__NAME);
    if (m_hIpcEvent == NULL) {
        DWORD winErr = GetLastError();
        QString err = QString("CreateEvent 失败，错误码: %1").arg(winErr);
        qCritical() << "❌ [IpcManager]" << err;
        cleanup();
        return err;
    }

    // 3. 映射物理内存
    m_pSharedData = (SharedData*)MapViewOfFile(m_hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    if (m_pSharedData == nullptr) {
        DWORD winErr = GetLastError();
        QString err = QString("MapViewOfFile 失败，错误码: %1").arg(winErr);
        qCritical() << "❌ [IpcManager]" << err;
        cleanup();
        return err;
    }

    // 4. 初始化内存数据
    if (!alreadyExists) {
        qDebug() << "   └── ✅ [IpcManager] 成功创建新共享内存";
        memset(m_pSharedData, 0, sizeof(SharedData));
    } else {
        qDebug() << "   └── ✅ [IpcManager] 成功附加到现有共享内存";
    }

    // 5. 设置 Qt 事件通知器
    if (!m_eventNotifier) {
        m_eventNotifier = new QWinEventNotifier(m_hIpcEvent, this);
        connect(m_eventNotifier, &QWinEventNotifier::activated, this, &IpcManager::onIpcMessageReceived);
    }
    m_eventNotifier->setEnabled(true);

    qDebug() << "✅ [IpcManager] IPC 系统初始化完成";
    return QString();
}

void IpcManager::resetIpc()
{
    if (m_pSharedData) {
        memset(m_pSharedData, 0, sizeof(SharedData));
        qDebug() << "🧹 [IPC] 共享内存内容已重置，映射指针保持有效";
    }
}

bool IpcManager::sendIpcBufferMessage(IpcMessageType type, const void *data, quint32 size)
{
    // 1. 检查共享内存是否就绪
    if (!m_pSharedData) {
        qWarning() << "❌ [IPC] 共享内存未连接";
        return false;
    }

    // 2. 判定优先级
    bool isFast = NetworkManager::instance().isFastMessage(type);

    // 3. 根据优先级选择对应的索引和缓冲区 (L2D 通道)
    volatile LONG *pWriteIdx = isFast ? &m_pSharedData->fast_write_index_l2d : &m_pSharedData->slow_write_index_l2d;
    volatile LONG *pReadIdx  = isFast ? &m_pSharedData->fast_read_index_l2d  : &m_pSharedData->slow_read_index_l2d;
    MessageSlot   *pBuffer   = isFast ? m_pSharedData->fast_buffer_l2d       : m_pSharedData->slow_buffer_l2d;

    // 4. 计算索引并检查溢出
    LONG writeIdx = *pWriteIdx;
    LONG readIdx  = *pReadIdx;
    LONG nextWriteIdx = (writeIdx + 1) % BUFFER_SIZE;

    if (nextWriteIdx == readIdx) {
        qWarning() << QString("⚠ [IPC %1] 缓冲区已满！丢弃类型: %2")
                          .arg(isFast ? "FAST" : "SLOW").arg(type);
        return false;
    }

    // 5. 获取槽位并填充数据
    MessageSlot &slot = pBuffer[writeIdx];
    slot.type = type;
    slot.size = size;
    slot.socket = 0;
    slot.isFast = isFast;

    if (data && size > 0) {
        // 限制最大长度
        quint32 copySize = qMin(size, (quint32)MAX_PACKET_SIZE);
        memcpy(slot.payload.data, data, copySize);
        // 字符串安全处理
        if (copySize < MAX_PACKET_SIZE) {
            slot.payload.data[copySize] = '\0';
        }
    }

    // 6. 原子提交写入索引
    InterlockedExchange(pWriteIdx, nextWriteIdx);

    // 7. 发送信号（如果有事件对象需要触发）
    SetEvent(m_hIpcEvent);

    qDebug().noquote() << QString("📤 [IPC %1] 指令入队 | 类型: %2 | 大小: %3")
                              .arg(isFast ? "FAST" : "SLOW")
                              .arg(type)
                              .arg(size);
    return true;
}

void IpcManager::onIpcMessageReceived()
{
    DebugHelper::recordTreeLog("ipc_Message", "▼ [Signal] QWinEventNotifier 捕捉到信号", 0);

    m_eventNotifier->setEnabled(false);
    auto *sharedData = IpcManager::instance().m_pSharedData;

    if (!sharedData) {
        DebugHelper::recordTreeLog("ipc_Message", "  ❌ [Error] 共享内存指针无效，尝试重连...", 1);
        if (!IpcManager::instance().initIpcManager().isEmpty()) {
            DebugHelper::recordTreeLog("ipc_Message", "  ❌ [Error] 重连初始化失败", 1, true);
            m_eventNotifier->setEnabled(true);
            return;
        }
        sharedData = IpcManager::instance().m_pSharedData;
        DebugHelper::recordTreeLog("ipc_Message", "  ✅ [Success] 共享内存已重新挂载", 1);
    }

    int fastWaiting = (sharedData->fast_write_index_d2l - sharedData->fast_read_index_d2l + BUFFER_SIZE) % BUFFER_SIZE;
    int slowWaiting = (sharedData->slow_write_index_d2l - sharedData->slow_read_index_d2l + BUFFER_SIZE) % BUFFER_SIZE;

    DebugHelper::recordTreeLog("ipc_Message",
                               QString("  🔍 缓冲区快照: Fast(R:%1, W:%2, Pending:%3) | Slow(R:%4, W:%5, Pending:%6)")
                                   .arg(sharedData->fast_read_index_d2l).arg(sharedData->fast_write_index_d2l).arg(fastWaiting)
                                   .arg(sharedData->slow_read_index_d2l).arg(sharedData->slow_write_index_d2l).arg(slowWaiting), 1);

    bool hasFast = (fastWaiting > 0);
    bool hasSlow = (slowWaiting > 0);

    if (hasFast || hasSlow) {
        DebugHelper::recordTreeLog("ipc_Message", "┌─ 🔔 开始处理同步任务", 0);
    } else {
        DebugHelper::recordTreeLog("ipc_Message", "└─ ℹ 信号触发但缓冲区为空 (可能是重复信号或已由前序逻辑排空)", 0, true);
        m_eventNotifier->setEnabled(true);
        return;
    }

    // 3. 处理快速通道 (Fast Buffer)
    int fastCount = 0;
    while (sharedData->fast_read_index_d2l != sharedData->fast_write_index_d2l)
    {
        LONG currentIdx = sharedData->fast_read_index_d2l;
        const MessageSlot &msg = sharedData->fast_buffer_d2l[currentIdx];

        dispatchIpcBufferMessage(msg);

        LONG next_read = (currentIdx + 1) % BUFFER_SIZE;
        InterlockedExchange(&sharedData->fast_read_index_d2l, next_read);
        fastCount++;
    }
    if (fastCount > 0) {
        DebugHelper::recordTreeLog("ipc_Message", QString("├─ ⚡ [Fast] 高优先级队列清空: %1 个").arg(fastCount), 1);
    }

    // 4. 处理慢速通道 (Slow Buffer)
    int slowProcessed = 0;
    while (sharedData->slow_read_index_d2l != sharedData->slow_write_index_d2l && slowProcessed < 100)
    {
        LONG currentIdx = sharedData->slow_read_index_d2l;
        const MessageSlot &msg = sharedData->slow_buffer_d2l[currentIdx];

        dispatchIpcBufferMessage(msg);

        LONG next_read = (currentIdx + 1) % BUFFER_SIZE;
        InterlockedExchange(&sharedData->slow_read_index_d2l, next_read);
        slowProcessed++;
    }

    if (slowProcessed > 0) {
        DebugHelper::recordTreeLog("ipc_Message", QString("├─ 🐢 [Slow] 慢速队列同步完成: %1/%2 个").arg(slowProcessed).arg(slowWaiting), 1);
        if (slowWaiting > 100) {
            DebugHelper::recordTreeLog("ipc_Message", "├─ ⚠ [Limit] 队列堆积，单次处理上限已达", 1);
        }
    }

    DebugHelper::recordTreeLog("ipc_Message", "└─ ✨ 本次同步流程结束", 0, true);

    m_eventNotifier->setEnabled(true);
}

void IpcManager::dispatchIpcBufferMessage(const MessageSlot &message)
{
    // 基础长度校验
    if (message.size > MAX_PACKET_SIZE) {
        DebugHelper::recordTreeLog("ipc_Message", QString("❌ [Error] 非法包大小: %1 (已丢弃)").arg(message.size), 1);
        return;
    }

    switch (message.type) {
    case MSG_TYPE_TRANSLATE_REQUEST:
    {
        const auto *payload = reinterpret_cast<const NotifyTranslatePayload*>(message.payload.data);

        quint32 pid = payload->pid;
        quint32 flag = payload->flag;
        quint32 extraScope = payload->extraScope;

        QString rawText = QString::fromUtf8(payload->message,
                                            strnlen(payload->message, sizeof(payload->message))).trimmed();
        QString language = SettingsManager::instance().languageCode();

        // 噪音拦截日志
        static QRegularExpression noiseRegex("[\\p{L}\\p{N}]");
        if (!rawText.contains(noiseRegex)) {
            // 噪音文本通常是垃圾信息，使用轻量日志记录
            DebugHelper::recordTreeLog("chat_translate", QString("ℹ 忽略无意义噪音: \"%1\"").arg(rawText), 1);
            break;
        }

        DebugHelper::recordTreeLog("chat_translate", "┌─ 📥 [Translate] 捕获游戏内聊天文本", 0);
        DebugHelper::recordTreeLog("chat_translate", QString("├─ 来源: PID=%1 | 范围: %2").arg(pid).arg(extraScope), 1);
        DebugHelper::recordTreeLog("chat_translate", QString("├─ 文本: \"%1\"").arg(rawText), 1);

        QMetaObject::invokeMethod(&TranslateManager::instance(),
                                  "requestTranslationWithMetadata",
                                  Qt::QueuedConnection,
                                  Q_ARG(quint32, pid),
                                  Q_ARG(quint32, flag),
                                  Q_ARG(quint32, extraScope),
                                  Q_ARG(QString, rawText),
                                  Q_ARG(QString, language));

        DebugHelper::recordTreeLog("chat_translate", "└─ 🚀 任务已进入异步翻译流水线", 0, true);
        break;
    }

    case MSG_TYPE_TRANSLATE_MAPPED:
    {
        const auto *payload = reinterpret_cast<const TranslatedResultPayload*>(message.payload.data);
        QString translatedMessage = QString::fromUtf8(payload->translatedMessage).trimmed();

        DebugHelper::recordTreeLog("ipc_mapped", "┌─ ✅ [Mapped] 接收到 DLL 端内存改写确认", 0);
        DebugHelper::recordTreeLog("ipc_mapped", QString("├─ 目标: PID=%1").arg(payload->pid), 1);
        DebugHelper::recordTreeLog("ipc_mapped", QString("├─ 内容: \"%1\"").arg(translatedMessage), 1);

        NetworkManager::instance().sendTranslatedMessage(payload->pid, payload->flag, payload->extraScope, translatedMessage);

        DebugHelper::recordTreeLog("ipc_mapped", "└─ 🌐 译文重发指令已下发至网络层", 0, true);
        break;
    }

    default:
        DebugHelper::recordTreeLog("ipc_Message", QString("⚠ [Warning] 收到未知 IPC 协议头: %1").arg(message.type), 1);
        break;
    }
}