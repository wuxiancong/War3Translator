#ifndef HOOKMANAGER_H
#define HOOKMANAGER_H

#include <Windows.h>
#include <QObject>
#include <QString>
#include <QVector>

#ifndef WAR3PROC_EXE_NAME
#define WAR3PROC_EXE_NAME "War3.exe"
#endif

// 函数导出信息结构体
struct ExportFunctionInfo {
    QString name;           // 函数名
    QString ordinalName;    // 序号名（如 "#123"）
    DWORD ordinal;          // 序号
    DWORD rva;              // 相对虚拟地址
    bool isByName;          // 是否按名称导出
};

enum class HookState {
    IDLE,
    INSTALLING,
    INSTALLED,
    UNINSTALLING
};

class HookManager : public QObject
{
    Q_OBJECT

public:
    // 禁止拷贝和赋值
    HookManager(const HookManager&) = delete;
    HookManager &operator=(const HookManager&) = delete;

    // 获取单例实例
    static HookManager &instance();

    // DLL 注入函数
    bool injectDLL(const QString &processName, const QString &dllPath);
    bool injectDLL(DWORD processId, const QString &dllPath);

    // 注入 translator.dll 到指定进程
    bool injectTranslatorDll(const QString &processName = WAR3PROC_EXE_NAME);
    bool injectTranslatorDll(DWORD processId);

    // 检查函数
    bool isDllInjected(const QString &processName, const QString &dllName);
    bool isDllInjected(DWORD processId, const QString &dllName);
    static void checkCurrentProcessInjectionStatic();

    // 远程调用 translator.dll 中的函数
    bool callHookFunction(const QString &processName, const QString &functionName);
    bool callHookFunction(DWORD processId, const QString &functionName);

    // 远程钩子相关方法
    bool initialize(const QString &processName = WAR3PROC_EXE_NAME);
    bool initialize(DWORD processId);
    bool installAllHooks(const QString &processName = WAR3PROC_EXE_NAME);
    bool installAllHooks(DWORD processId);
    bool uninstallAllHooks(const QString &processName = WAR3PROC_EXE_NAME);
    bool uninstallAllHooks(DWORD processId);
    HookState getHookStatus(const QString &processName = WAR3PROC_EXE_NAME);
    HookState getHookStatus(DWORD processId);
    bool isInstalledOrInstalling(const QString &processName = WAR3PROC_EXE_NAME);
    bool isInstalledOrInstalling(DWORD processId);
    QString hookStatusToString(HookState status);

    // 获取远程DLL的导出函数列表
    QVector<ExportFunctionInfo> getRemoteExportFunctions(const QString &processName, const QString &dllName);
    QVector<ExportFunctionInfo> getRemoteExportFunctions(DWORD processId, const QString &dllName);

    // 获取特定函数的导出信息
    ExportFunctionInfo getRemoteFunctionInfo(const QString &processName, const QString &dllName, const QString &functionName);
    ExportFunctionInfo getRemoteFunctionInfo(DWORD processId, const QString &dllName, const QString &functionName);

    // 远程调用实现
    bool callRemoteFunction(DWORD processId, const QString &dllName, const QString &functionName, LPCVOID lpParameter = nullptr, SIZE_T dwSize = 0);

signals:
    void injectionStarted(const QString &processName);
    void injectionFinished(bool success, const QString &message);
    void errorOccurred(const QString &errorMessage);
    void hookFunctionCalled(bool success, const QString &functionName, const QString &message);

private:
    // 私有构造函数
    explicit HookManager(QObject *parent = nullptr);
    ~HookManager();

    // 注入实现
    bool injectDLLInternal(DWORD processId, const QString &dllPath);

    // 获取 translator.dll 路径
    QString getTranslatorDllPath();

    // 获取函数地址
    FARPROC getFunctionAddress(const QString &dllPath, const QString &functionName);

    // 解析远程DLL的导出表
    QVector<ExportFunctionInfo> parseRemoteExportTable(HANDLE hProcess, HMODULE hRemoteModule);
};

#endif // HOOKMANAGER_H
