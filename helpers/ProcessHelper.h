#ifndef PROCESSHELPER_H
#define PROCESSHELPER_H

#include <windows.h>
#include <winternl.h>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <QVariant>
#include <QProcess>
#include <QSharedPointer>
#include <QScopedPointer>

#define GAME_DLL_NAME_LEN 8                                                 /* Game.dll 的长度 */
#define MAX_DLL_NAME_LEN 11                                                 /* 替换 Game.dll 最大长度 */
#define GAME_DLL_NAME "Game.dll"                                            /* Game.dll 的名称（以防更改）*/

typedef LSTATUS(NTAPI *PNtQueryInformationProcess)(
    IN	HANDLE ProcessHandle,
    IN	PROCESSINFOCLASS ProcessInformationClass,
    OUT	PVOID ProcessInformation,
    IN	ULONG ProcessInformationLength,
    OUT	PULONG ReturnLength	OPTIONAL
    );
static PNtQueryInformationProcess g_pNtQueryInformationProcess = nullptr;

// 自动管理句柄的 RAII 类
class ScopeHandle
{
public:
    explicit ScopeHandle(HANDLE handle = nullptr) : m_handle(handle) {}

    ~ScopeHandle() {
        if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
        }
    }

    // 禁止拷贝
    ScopeHandle(const ScopeHandle&) = delete;
    ScopeHandle& operator=(const ScopeHandle&) = delete;

    // 允许移动
    ScopeHandle(ScopeHandle&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    ScopeHandle& operator=(ScopeHandle&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    // 获取原始句柄
    HANDLE get() const { return m_handle; }

    // 检查句柄是否有效
    bool isValid() const {
        return m_handle && m_handle != INVALID_HANDLE_VALUE;
    }

    // 重置句柄
    void reset(HANDLE handle = nullptr) {
        if (m_handle && m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
        }
        m_handle = handle;
    }

    // 释放所有权（不关闭句柄）
    HANDLE release() {
        HANDLE handle = m_handle;
        m_handle = nullptr;
        return handle;
    }

private:
    HANDLE m_handle;
};

// 模块信息结构体
struct ModuleInfo
{
    QString name;
    quintptr baseAddress;
    quint32 size;
    QString path;
};

// 主辅助类 - 单例模式
class ProcessHelper : public QObject
{
    Q_OBJECT

private:
    explicit ProcessHelper(QObject *parent = nullptr);
    ~ProcessHelper();
    // 声明 QScopedPointerDeleter 为友元
    template<typename T> friend struct QScopedPointerDeleter;
public:
    // 禁止拷贝和赋值
    ProcessHelper(const ProcessHelper&) = delete;
    ProcessHelper& operator=(const ProcessHelper&) = delete;

    // 获取单例实例
    static ProcessHelper &instance();

    // 重启当前应用程序
    Q_INVOKABLE static void restartApplication();

    // 检查进程是否在运行
    Q_INVOKABLE bool isProcessRunning(const QString &processName);

    // 强制关闭指定进程名的进程
    Q_INVOKABLE bool forceTerminateProcess(const QString &processName);
    // 关闭所有同名进程
    Q_INVOKABLE bool forceTerminateAllProcesses(const QString &processName);

    // 注入进程
    bool injectHookDll(const QString &gamePath = "");

    // 允许调试权限
    bool enableDebugPrivilege();

    // 根据进程名获取 PID
    DWORD getProcessIdByName(const QString &processName);

    // 查找最后的进程 PID
    DWORD findLatestWar3Process();

    // 根据PID获取 进程名
    QString getProcessNameById(DWORD processId);

    // 打开进程
    bool openProcess(const QString &processName);
    bool openProcessByPid(DWORD pid);

    // 关闭进程
    bool forceTerminateProcessByPid(DWORD pid);
    bool forceTerminateProcessEx(const QString &processName, bool forceIfHung);

    // 关闭当前已打开的进程
    bool terminateCurrentProcess();

    // 关闭所有同名进程（除了当前进程）
    bool forceTerminateAllProcessesExcludeSelf(const QString &processName);

    // 获取所有同名进程的PID列表
    QVector<DWORD> getAllProcessIdsByName(const QString &processName);

    // 批量关闭进程（通过PID列表）
    bool forceTerminateProcesses(const QVector<DWORD> &pids);

    // 智能关闭 - 尝试先正常关闭，再强制关闭
    bool smartTerminateProcesses(const QString &processName, int timeoutMs = 5000);

    // 关闭进程树（包括子进程）
    bool forceTerminateProcessTree(const QString &processName);

    // 获取模块基地址 - 主要方法
    quintptr getModuleBaseAddress(const QString &moduleName);

    // 获取模块信息
    QVariantMap getModuleInfo(const QString &moduleName);

    // 获取所有模块信息
    QVector<QVariantMap> getAllModules();

    // 获取所有模块名称列表
    QVector<QString> getModuleList();

    // 获取进程句柄
    HANDLE getProcessHandle(DWORD pid, DWORD desiredAccess = PROCESS_ALL_ACCESS);

    // 获取线程句柄
    quintptr getMainThreadHandle();
    QVector<quintptr> getAllThreadHandles();

    // 关闭当前进程句柄
    void closeCurrentProcess();

    // 获取当前进程信息
    DWORD getCurrentPid() const { return m_currentPid; }
    QString getCurrentProcessName() const { return m_currentProcessName; }
    bool isProcessOpened() const { return m_processHandle.isValid(); }

    // 进程枚举相关
    QVector<QVariantMap> getAllProcesses();

    // 查询进程信息
    PNtQueryInformationProcess getNtQueryInformationProcess();

    // 获取基地址
    DWORD getProcessBaseAddressByWinApi(HANDLE hProcess);
    DWORD getProcessBaseAddress(HANDLE hThread, HANDLE hProcess);
    DWORD getProcessBaseAddressByRegister(HANDLE hThread, HANDLE hProcess);

    // 读取远程进程内存
    bool readRemoteMemory(HANDLE hProcess, LPCVOID remoteAddress, LPVOID localBuffer, SIZE_T size);

    // QProcess 相关功能
    QVariantMap getQProcessBaseAddressWithModuleName(QProcess *process, const QString &moduleName = "");
    QVector<QVariantMap> getProcessInfoFromQProcess(QProcess *process);
    QString getQProcessMainModuleBaseAddress(QProcess *process);
    QVariantMap getQProcessDetailedInfo(QProcess *process);
    DWORD_PTR getQProcessBaseAddress(QProcess *process);
    QVariantMap getQProcessStatus(QProcess *process);
    QString getCurrentAttachedProcessInfo() const;
    bool attachToQProcess(QProcess *process);

    // 获取真正的 PROCESS_INFORMATION 类型的进程信息
    PROCESS_INFORMATION getQProcessInformation(QProcess *process);

    // 通过进程ID获取 PROCESS_INFORMATION
    PROCESS_INFORMATION getProcessInformationByPid(DWORD pid);
private:
    ScopeHandle m_processHandle;
    DWORD m_currentPid;
    QString m_currentProcessName;

    // 单例相关
    static QMutex m_mutex;
    static QScopedPointer<ProcessHelper> s_instance;

    // 内部辅助方法
    QVector<ModuleInfo> getAllModulesInternal();
    ModuleInfo getModuleInfoInternal(const QString &moduleName);
    static BOOL CALLBACK enumWindowsProcClose(HWND hwnd, LPARAM lParam);
    static BOOL CALLBACK enumWindowsProcFindPid(HWND hwnd, LPARAM lParam);
};

#endif // PROCESSHELPER_H
