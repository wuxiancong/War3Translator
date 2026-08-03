#ifndef GAMEMONITOR_H
#define GAMEMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>

enum SetupStep {
    InitIPC = 0,    // 0. 初始化 Qt 端 IPC
    CallInit,       // 1. 调用 DLL 的 initialize
    CallHooks,      // 2. 调用 DLL 的 installAllHooks
    Finished        // 3. 完成
};

class GameMonitor : public QObject
{
    Q_OBJECT
public:
    // 禁止拷贝和赋值
    GameMonitor(const GameMonitor&) = delete;
    GameMonitor &operator=(const GameMonitor&) = delete;

    // 获取单例实例
    static GameMonitor &instance();

    Q_INVOKABLE void cleanup();
    Q_INVOKABLE bool checkGameStatus();
    Q_INVOKABLE bool checkHookStatusAndInstall(bool checkImmediately = false);

    void initGameMonitor();
    void startMonitoring();
    void maintainHeartbeat();

private:
    explicit GameMonitor(QObject *parent = nullptr);
    ~GameMonitor();

private slots:
    void onDllCheckTimeout();
    void onSetupTimeout();
    void onTimerTimeout() { checkHookStatusAndInstall(false); }

signals:
    void gameStateChanged(bool isRunning, int stateFlag);

private:
    QString m_targetExeName;
    SetupStep m_currentStep;
    int m_installRetryCount;
    int m_injectionDelay;
    int m_monitorInterval;
    bool m_lastGameRunning;
    bool m_isFullyInitialized;
    std::atomic<bool> m_isInjecting{false};
    // 定时器
    QTimer *m_timer;
    QTimer *m_setupTimer;
    QTimer *m_gameStatusTimer;
    QTimer *m_dllCheckTimer;
};

#endif // GAMEMONITOR_H
