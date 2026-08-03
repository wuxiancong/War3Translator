#include "../helpers/ProcessHelper.h"
#include "../monitors/GameMonitor.h"
#include "../managers/HookManager.h"
#include "../managers/IpcManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QSettings>
#include <QFileInfo>
#include <QThread>
#include <QDebug>
#include <QDir>

GameMonitor &GameMonitor::instance()
{
    static GameMonitor instance;
    return instance;
}

GameMonitor::GameMonitor(QObject *parent)
    : QObject(parent)
    , m_targetExeName(QString())
    , m_installRetryCount(0)
    , m_injectionDelay(1000)
    , m_monitorInterval(1000)
    , m_lastGameRunning(false)
    , m_isInjecting(false)
    , m_isFullyInitialized(false)
    , m_currentStep(InitIPC)
    , m_timer(nullptr)
    , m_setupTimer(nullptr)
    , m_gameStatusTimer(nullptr)
    , m_dllCheckTimer(nullptr)
{
    m_monitorInterval = 500;
    m_injectionDelay = 500;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GameMonitor::onTimerTimeout);

    m_setupTimer = new QTimer(this);
    m_setupTimer->setInterval(500);
    connect(m_setupTimer, &QTimer::timeout, this, &GameMonitor::onSetupTimeout);

    m_dllCheckTimer = new QTimer(this);
    m_dllCheckTimer->setInterval(500);
    connect(m_dllCheckTimer, &QTimer::timeout, this, &GameMonitor::onDllCheckTimeout);

    m_gameStatusTimer = new QTimer(this);
    m_gameStatusTimer->setInterval(500);
    connect(m_gameStatusTimer, &QTimer::timeout, this, &GameMonitor::checkGameStatus);
    m_gameStatusTimer->start();

    initGameMonitor();
}

GameMonitor::~GameMonitor()
{
    cleanup();
}

void GameMonitor::cleanup()
{
    qDebug() << "🕵 GameMonitor 停止所有监视任务...";

    if (m_timer) m_timer->stop();
    if (m_setupTimer) m_setupTimer->stop();
    if (m_gameStatusTimer) m_gameStatusTimer->stop();
    if (m_dllCheckTimer) m_dllCheckTimer->stop();

    this->disconnect();
}

void GameMonitor::initGameMonitor()
{
    if (m_timer->isActive()) return;
    m_monitorInterval = 500;
    m_injectionDelay = 500;
    startMonitoring();
}

void GameMonitor::startMonitoring()
{
    if (m_timer->isActive()) return;
    m_timer->setInterval(m_monitorInterval);
    m_timer->start();
}

bool GameMonitor::checkGameStatus()
{
    QString target = m_targetExeName.isEmpty() ? "war3.exe" : m_targetExeName;

    // 1. 获取当前实时状态
    bool isRunning = ProcessHelper::instance().isProcessRunning(target);

    if (isRunning) {
        // --- 游戏启动那一瞬间的逻辑 ---
        if (!m_lastGameRunning) {
            m_installRetryCount = 0;
            if (!m_dllCheckTimer->isActive()) m_dllCheckTimer->start();

            // 只有第一次检测到启动，发一次 true
            emit gameStateChanged(true, 0);
        }
    } else {
        // --- 进程不存在时的逻辑 ---

        // 1. 信号：进程不存在时，每一秒都持续发送 false
        emit gameStateChanged(false, -1);

        // 2. 清理：只有当状态刚从运行变为不运行的那一刻，执行一次清理
        if (m_lastGameRunning) {
            if (m_dllCheckTimer->isActive()) m_dllCheckTimer->stop();
            if (m_setupTimer->isActive()) m_setupTimer->stop();
            QMetaObject::invokeMethod(&IpcManager::instance(), "resetIpc", Qt::QueuedConnection);
        }
    }

    // 更新最后一次状态记录
    m_lastGameRunning = isRunning;
    return isRunning;
}

bool GameMonitor::checkHookStatusAndInstall(bool checkImmediately)
{
    QString realTarget = m_targetExeName.isEmpty() ? "war3.exe" : m_targetExeName;

    // 1. 基础检查
    if (!ProcessHelper::instance().isProcessRunning(realTarget)) {
        if (m_isFullyInitialized) qDebug() << "🔴 [Monitor] 进程已消失，重置状态";
        m_isFullyInitialized = false;
        m_isInjecting = false;
        return false;
    }

    // 2. 状态检查
    if (m_isFullyInitialized && !checkImmediately) {
        maintainHeartbeat();
        return true;
    }

    // 3. 尝试获取注入锁
    if (m_isInjecting.exchange(true)) {
        qDebug() << "⚠ [Monitor] 注入锁占用中，跳过本次请求 (来源:" << (checkImmediately ? "立即" : "定时器") << ")";
        return true;
    }

    bool isDllPresent = HookManager::instance().isDllInjected(realTarget, "translator.dll");

    // --- 路径 A: 立即执行 (FastPath) ---
    if (checkImmediately) {
        qDebug() << "🚀 [Monitor] 触发快速注入路径...";
        m_timer->stop();
        if (!isDllPresent) ProcessHelper::instance().injectHookDll();

        QTimer::singleShot(20, this, [this]() {
            if (HookManager::instance().initialize() && HookManager::instance().installAllHooks()) {
                qDebug() << "✅ [Monitor] 快速路径注入并挂钩成功";
                m_isFullyInitialized = true;
            } else {
                qDebug() << "❌ [Monitor] 快速路径失败 (Init/Hook 环节)";
            }
            m_isInjecting = false;
            m_timer->start();
        });
        return true;
    }

    // --- 路径 B: 延迟注入序列 ---
    if (!isDllPresent) {
        qDebug() << "⏳ [Monitor] 检测到游戏运行，开启延迟注入序列 (" << m_injectionDelay << "ms)";
        m_timer->stop();

        QTimer::singleShot(m_injectionDelay, this, [this]() {
            QString target = m_targetExeName.isEmpty() ? "war3.exe" : m_targetExeName;
            if (!ProcessHelper::instance().isProcessRunning(target)) {
                m_isInjecting = false; m_timer->start(); return;
            }

            if (ProcessHelper::instance().injectHookDll()) {
                QTimer::singleShot(500, this, [this]() {
                    if (HookManager::instance().initialize()) {
                        QTimer::singleShot(500, this, [this]() {
                            m_isFullyInitialized = HookManager::instance().installAllHooks();
                            qDebug() << (m_isFullyInitialized ? "✅ [Monitor] 延迟注入序列成功" : "❌ [Monitor] 延迟安装钩子失败");
                            m_isInjecting = false; m_timer->start();
                        });
                    } else {
                        qDebug() << "❌ [Monitor] 延迟初始化失败";
                        m_isInjecting = false; m_timer->start();
                    }
                });
            } else {
                qDebug() << "❌ [Monitor] 注入 DLL 失败";
                m_isInjecting = false; m_timer->start();
            }
        });
    }
    // --- 路径 C: DLL已存在，仅补挂 ---
    else {
        qDebug() << "ℹ [Monitor] DLL已在线，执行快速补挂流程...";
        QTimer::singleShot(100, this, [this]() {
            if (HookManager::instance().initialize()) {
                m_isFullyInitialized = HookManager::instance().installAllHooks();
                qDebug() << "✅ [Monitor] 补挂完成，状态:" << m_isFullyInitialized;
            }
            m_isInjecting = false;
            m_timer->start();
        });
    }

    return true;
}

void GameMonitor::maintainHeartbeat()
{
    m_injectionDelay = 500;
    if (m_monitorInterval < 5000) {
        m_monitorInterval += 500;
    } else {
        m_monitorInterval = 2000;
    }
    m_timer->setInterval(m_monitorInterval);
}

void GameMonitor::onDllCheckTimeout()
{
    QString target = m_targetExeName.isEmpty() ? "war3.exe" : m_targetExeName;

    if (!ProcessHelper::instance().isProcessRunning(target)) {
        m_dllCheckTimer->stop();
        return;
    }

    if (HookManager::instance().isDllInjected(target, "translator.dll")) {
        qDebug() << "✅ [Step 0] 检测到 translator.dll 已注入，开始初始化序列...";

        // 停止检测定时器
        m_dllCheckTimer->stop();

        // 启动分步初始化定时器
        m_currentStep = InitIPC;
        m_setupTimer->start();
    }
}

void GameMonitor::onSetupTimeout()
{
    QString target = m_targetExeName.isEmpty() ? "war3.exe" : m_targetExeName;

    if (!ProcessHelper::instance().isProcessRunning(target)) {
        qDebug() << "🛑 初始化过程中游戏退出，停止序列";
        m_setupTimer->stop();
        m_installRetryCount = 0;
        QMetaObject::invokeMethod(&IpcManager::instance(), "resetIpc", Qt::QueuedConnection);
        return;
    }

    if (m_installRetryCount >= 3) {
        qCritical() << "❌ [Monitor] 安装尝试已达上限(3次)且均失败。停止自动安装序列以保护系统。";
        m_setupTimer->stop();
        return;
    }

    bool success = false;

    switch (m_currentStep) {
    case InitIPC: {
        qDebug() << "⚙ [Step 1] 初始化 Qt 端共享内存...";
        QString ipcError;
        if (IpcManager::instance().thread() == QThread::currentThread()) {
            IpcManager::instance().initIpcManager();
        } else {
            QMetaObject::invokeMethod(&IpcManager::instance(), "initIpcManager",
                                      Qt::BlockingQueuedConnection);
        }
        if (!ipcError.isEmpty()) {
            qDebug() << "共享内存初始化失败: " + ipcError;
        } else {
            qDebug() << "共享内存初始化成功";
        }

        success = true;
        if (success) {
            m_currentStep = CallInit;
        }
        break;
    }
    case CallInit: {
        if (HookManager::instance().callHookFunction(target, "initialize")) {
            qDebug() << "⚙ [Step 2] 远程调用 initialize 成功";
            m_currentStep = CallHooks;
        } else {
            m_installRetryCount++;
            qDebug() << "⚠ [Step 2] 远程调用 initialize 失败/忙碌，下个周期重试...";
        }
        break;
    }

    case CallHooks: {
        if (HookManager::instance().callHookFunction(target, "installAllHooks")) {
            qDebug() << "✅ [Step 3] 远程调用 installAllHooks 成功，挂钩完成！";
            m_currentStep = Finished;
        } else {
            m_installRetryCount++;
            qDebug() << "⚠ [Step 3] 远程调用 installAllHooks 失败，下个周期重试...";
        }
        break;
    }

    case Finished: {
        qDebug() << "🎉 所有初始化步骤执行完毕，停止定时器";
        m_setupTimer->stop();
        m_installRetryCount = 0;
        break;
    }
    }
}
