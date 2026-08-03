#include "../managers/HookManager.h"
#include "../helpers/ProcessHelper.h"
#include "../helpers/FileHelper.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>
#include <QDir>

HookManager &HookManager::instance() {
    static HookManager instance;
    return instance;
}

HookManager::HookManager(QObject *parent) : QObject(parent)
{

}

HookManager::~HookManager()
{

}

bool HookManager::injectDLL(const QString &processName, const QString &dllPath)
{
    emit injectionStarted(processName);

    DWORD processId = ProcessHelper::instance().getProcessIdByName(processName);
    if (processId == 0) {
        emit errorOccurred(QString("未找到进程: %1").arg(processName));
        emit injectionFinished(false, "进程未找到");
        return false;
    }

    return injectDLL(processId, dllPath);
}

bool HookManager::injectDLL(DWORD processId, const QString &dllPath)
{
    if (!ProcessHelper::instance().openProcessByPid(processId)) {
        emit errorOccurred("无法打开指定进程");
        emit injectionFinished(false, "进程无法访问");
        return false;
    }

    QFileInfo dllFileInfo(dllPath);
    if (!dllFileInfo.exists()) {
        emit errorOccurred(QString("DLL 文件不存在: %1").arg(dllPath));
        emit injectionFinished(false, "DLL 文件不存在");
        return false;
    }

    QString dllName = dllFileInfo.fileName();
    if (isDllInjected(processId, dllName)) {
        emit injectionFinished(true, "DLL 已注入");
        return true;
    }

    bool result = injectDLLInternal(processId, dllPath);
    emit injectionFinished(result, result ? "DLL 注入成功" : "DLL 注入失败");
    return result;
}

bool HookManager::injectDLLInternal(DWORD processId, const QString &dllPath)
{
    qDebug() << "=== 开始注入过程 ===";

    // 验证目标进程名
    QString targetProcessName = ProcessHelper::instance().getProcessNameById(processId);
    if (targetProcessName.isEmpty()) {
        qDebug() << "错误：无法获取目标进程名";
        emit errorOccurred("无法获取目标进程名");
        return false;
    }

    if (targetProcessName.compare("War3.exe", Qt::CaseInsensitive) != 0) {
        qDebug() << "跳过注入到非目标进程:" << targetProcessName;
        emit errorOccurred(QString("跳过注入到非目标进程: %1").arg(targetProcessName));
        return false;
    }

    qDebug() << "准备注入到目标进程:" << targetProcessName << "PID:" << processId;

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) {
        DWORD error = GetLastError();
        qDebug() << "无法打开进程，错误代码:" << error;

        // 尝试使用较少的权限
        hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                   PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                               FALSE, processId);
        if (!hProcess) {
            error = GetLastError();
            qDebug() << "使用标准权限也无法打开进程，错误代码:" << error;
            emit errorOccurred(QString("无法打开进程, 错误代码: %1").arg(error));
            return false;
        }
        qDebug() << "使用标准权限成功打开进程";
    } else {
        qDebug() << "使用完全权限成功打开进程句柄";
    }

    // 转换路径为宽字符串
    std::wstring wideDllPath = dllPath.toStdWString();
    size_t pathSize = (wideDllPath.length() + 1) * sizeof(wchar_t);
    qDebug() << "DLL路径:" << dllPath << "路径大小:" << pathSize;

    // 在目标进程中分配内存
    LPVOID pRemoteMemory = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteMemory) {
        DWORD error = GetLastError();
        qDebug() << "无法在目标进程中分配内存，错误代码:" << error;
        emit errorOccurred("无法在目标进程中分配内存");
        CloseHandle(hProcess);
        return false;
    }
    qDebug() << "成功分配远程内存:" << pRemoteMemory;

    // 写入 DLL 路径到目标进程
    SIZE_T bytesWritten = 0;
    BOOL writeResult = WriteProcessMemory(hProcess, pRemoteMemory, wideDllPath.c_str(), pathSize, &bytesWritten);
    if (!writeResult || bytesWritten != pathSize) {
        DWORD error = GetLastError();
        qDebug() << "无法写入 DLL 路径到目标进程，错误代码:" << error
                 << "已写入字节:" << bytesWritten << "期望字节:" << pathSize;
        emit errorOccurred("无法写入 DLL 路径到目标进程");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    qDebug() << "成功写入DLL路径到远程内存，字节数:" << bytesWritten;

    // 获取 LoadLibraryW 函数地址
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        DWORD error = GetLastError();
        qDebug() << "无法获取 kernel32.dll 模块句柄，错误代码:" << error;
        emit errorOccurred("无法获取 kernel32.dll 模块句柄");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    FARPROC pLoadLibrary = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibrary) {
        DWORD error = GetLastError();
        qDebug() << "无法获取 LoadLibraryW 地址，错误代码:" << error;
        emit errorOccurred("无法获取 LoadLibraryW 地址");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    qDebug() << "成功获取 LoadLibraryW 地址:" << reinterpret_cast<void*>(pLoadLibrary);

    // 创建远程线程调用 LoadLibraryW
    qDebug() << "创建远程线程...";
    HANDLE hRemoteThread = CreateRemoteThread(hProcess,
                                              NULL,
                                              0,
                                              (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                              pRemoteMemory,
                                              0,
                                              NULL);
    if (!hRemoteThread) {
        DWORD error = GetLastError();
        qDebug() << "无法创建远程线程，错误代码:" << error;
        emit errorOccurred("无法创建远程线程");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }
    qDebug() << "成功创建远程线程，线程句柄:" << hRemoteThread;

    // 等待线程结束（设置超时时间）
    qDebug() << "等待远程线程结束...";
    DWORD waitResult = WaitForSingleObject(hRemoteThread, 10000); // 10秒超时

    if (waitResult == WAIT_TIMEOUT) {
        qDebug() << "远程线程执行超时";
        TerminateThread(hRemoteThread, 0);
        emit errorOccurred("远程线程执行超时");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hRemoteThread);
        CloseHandle(hProcess);
        return false;
    } else if (waitResult == WAIT_FAILED) {
        DWORD error = GetLastError();
        qDebug() << "等待线程失败，错误代码:" << error;
        emit errorOccurred("等待远程线程失败");
        VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
        CloseHandle(hRemoteThread);
        CloseHandle(hProcess);
        return false;
    }

    qDebug() << "远程线程执行完成，等待结果:" << waitResult;

    // 获取线程退出码（DLL 模块句柄）
    DWORD exitCode = 0;
    if (!GetExitCodeThread(hRemoteThread, &exitCode)) {
        DWORD error = GetLastError();
        qDebug() << "无法获取线程退出码，错误代码:" << error;
        exitCode = 0;
    }
    qDebug() << "远程线程退出码:" << exitCode;

    // 清理资源
    VirtualFreeEx(hProcess, pRemoteMemory, 0, MEM_RELEASE);
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);

    // 如果退出码为0，表示注入失败
    if (exitCode == 0) {
        qDebug() << "注入失败：远程线程返回0，DLL可能未加载";
        emit errorOccurred("远程线程返回0，DLL可能加载失败");
        return false;
    }

    qDebug() << "=== 注入成功 ===";
    return true;
}

// ==================== 检查函数 ====================

bool HookManager::isDllInjected(const QString &processName, const QString &dllName)
{
    DWORD processId = ProcessHelper::instance().getProcessIdByName(processName);
    return (processId != 0) && isDllInjected(processId, dllName);
}

bool HookManager::isDllInjected(DWORD processId, const QString &dllName)
{
    if (!ProcessHelper::instance().openProcessByPid(processId)) {
        return false;
    }

    QVariantMap moduleInfo = ProcessHelper::instance().getModuleInfo(dllName);
    return moduleInfo["isValid"].toBool();
}

// 检查当前进程是否被注入了 translator.dll
void HookManager::checkCurrentProcessInjectionStatic()
{
    HMODULE hModule = GetModuleHandleA("translator.dll");
    if (hModule != NULL) {
        qDebug() << "警告：当前进程（启动器）已被注入了 translator.dll";

        char processName[MAX_PATH];
        GetModuleFileNameA(NULL, processName, MAX_PATH);
        qDebug() << "当前进程:" << processName;
    } else {
        qDebug() << "当前进程未被注入 translator.dll";
    }
}

// ==================== Translator DLL 注入 ====================

bool HookManager::injectTranslatorDll(const QString &processName)
{
    QString dllPath = getTranslatorDllPath();
    if (dllPath.isEmpty()) {
        emit errorOccurred("无法找到 translator.dll 文件");
        emit injectionFinished(false, "translator.dll 未找到");
        return false;
    }

    qDebug() << "准备注入 translator.dll:" << dllPath << "到进程:" << processName;
    return injectDLL(processName, dllPath);
}

bool HookManager::injectTranslatorDll(DWORD processId)
{
    QString dllPath = getTranslatorDllPath();
    if (dllPath.isEmpty()) {
        emit errorOccurred("无法找到 translator.dll 文件");
        emit injectionFinished(false, "translator.dll 未找到");
        return false;
    }

    qDebug() << "准备注入 translator.dll:" << dllPath << "到进程ID:" << processId;
    return injectDLL(processId, dllPath);
}

QString HookManager::getTranslatorDllPath()
{
    // 方法1：在当前应用程序目录下查找
    QString appDir = QCoreApplication::applicationDirPath();
    QString dllPath1 = FileHelper::instance().combinePaths(appDir, "translator.dll");
    if (FileHelper::instance().fileExists(dllPath1)) {
        qDebug() << "找到 translator.dll 在应用程序目录:" << dllPath1;
        return dllPath1;
    }

    // 方法2：在当前工作目录下查找
    QString currentDir = QDir::currentPath();
    QString dllPath2 = FileHelper::instance().combinePaths(currentDir, "translator.dll");
    if (FileHelper::instance().fileExists(dllPath2)) {
        qDebug() << "找到 translator.dll 在工作目录:" << dllPath2;
        return dllPath2;
    }

    // 方法3：在应用程序目录的子目录中查找
    QStringList searchDirs = {"hooker"};
    for (const QString &dir : searchDirs) {
        QString searchPath = FileHelper::instance().combinePaths(appDir, dir);
        QString dllPath = FileHelper::instance().combinePaths(searchPath, "translator.dll");
        if (FileHelper::instance().fileExists(dllPath)) {
            qDebug() << "找到 translator.dll 在子目录:" << dllPath;
            return dllPath;
        }
    }

    // 方法4：递归搜索应用程序目录
    QDir appDirectory(appDir);
    QStringList dllFiles = appDirectory.entryList(QStringList() << "translator.dll", QDir::Files, QDir::Name);
    if (!dllFiles.isEmpty()) {
        QString foundPath = FileHelper::instance().combinePaths(appDir, dllFiles.first());
        qDebug() << "递归找到 translator.dll:" << foundPath;
        return foundPath;
    }

    qDebug() << "未找到 translator.dll 文件";
    qDebug() << "搜索路径:" << appDir;
    qDebug() << "工作目录:" << currentDir;

    return QString();
}

// ==================== 远程函数调用 ====================

bool HookManager::callHookFunction(const QString &processName, const QString &functionName)
{
    DWORD processId = ProcessHelper::instance().getProcessIdByName(processName);
    if (processId == 0) {
        emit errorOccurred(QString("未找到进程: %1").arg(processName));
        return false;
    }

    return callHookFunction(processId, functionName);
}

bool HookManager::callHookFunction(DWORD processId, const QString &functionName)
{
    if (!ProcessHelper::instance().openProcessByPid(processId)) {
        emit errorOccurred("无法打开指定进程");
        return false;
    }

    // 检查 translator.dll 是否已注入
    if (!isDllInjected(processId, "translator.dll")) {
        emit errorOccurred("translator.dll 未注入到目标进程");
        return false;
    }

    bool result = callRemoteFunction(processId, "translator.dll", functionName);

    QString message = result ?
                          QString("函数 %1 调用成功").arg(functionName) :
                          QString("函数 %1 调用失败").arg(functionName);

    emit hookFunctionCalled(result, functionName, message);
    return result;
}

bool HookManager::callRemoteFunction(DWORD processId, const QString &dllName, const QString &functionName,
                                     LPCVOID lpParameter, SIZE_T dwSize)
{
    // --- 1. 打开进程 ---
    HANDLE hProcess = ProcessHelper::instance().getProcessHandle(processId, PROCESS_ALL_ACCESS);
    if (!hProcess) {
        emit errorOccurred(QString("无法打开进程, 错误代码: %1").arg(GetLastError()));
        return false;
    }

    // --- 2. 获取模块和函数地址 ---

    // 使用 ProcessHelper 获取模块信息
    if (!ProcessHelper::instance().openProcessByPid(processId)) {
        emit errorOccurred("无法打开指定进程");
        CloseHandle(hProcess);
        return false;
    }
    QVariantMap moduleInfo = ProcessHelper::instance().getModuleInfo(dllName);
    if (!moduleInfo["isValid"].toBool()) {
        emit errorOccurred(QString("无法在目标进程中找到模块: %1").arg(dllName));
        CloseHandle(hProcess);
        return false;
    }
    quintptr baseAddress = moduleInfo["baseAddress"].toULongLong();

    // 获取函数地址（在本地进程中）
    QString translatorDllPath = getTranslatorDllPath();
    if (translatorDllPath.isEmpty()) {
        emit errorOccurred(QString("无法找到 %1 文件").arg(dllName));
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hLocalModule = LoadLibraryExW(translatorDllPath.toStdWString().c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!hLocalModule) {
        emit errorOccurred(QString("本地加载模块失败: %1, 错误: %2").arg(dllName).arg(GetLastError()));
        CloseHandle(hProcess);
        return false;
    }

    FARPROC pFunction = GetProcAddress(hLocalModule, functionName.toStdString().c_str());
    if (!pFunction) {
        emit errorOccurred(QString("无法找到函数: %1").arg(functionName));
        FreeLibrary(hLocalModule);
        CloseHandle(hProcess);
        return false;
    }

    DWORD functionOffset = (DWORD)pFunction - (DWORD)hLocalModule;
    FARPROC pRemoteFunction = (FARPROC)(baseAddress + functionOffset);

    // --- 3. 处理参数 ---
    LPVOID pRemoteParameter = NULL;

    if (lpParameter != nullptr && dwSize > 0)
    {
        // a. 分配内存
        pRemoteParameter = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteParameter) {
            emit errorOccurred("无法为远程函数参数分配内存");
            CloseHandle(hProcess);
            return false;
        }

        // b. 写入数据
        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, pRemoteParameter, lpParameter, dwSize, &bytesWritten) || bytesWritten != dwSize) {
            emit errorOccurred("无法写入远程函数参数");
            VirtualFreeEx(hProcess, pRemoteParameter, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }
    }

    // --- 4. 创建远程线程 ---
    // pRemoteParameter 在无参数时为 NULL，在有参数时为远程内存地址，正好符合 CreateRemoteThread 的要求
    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
                                              (LPTHREAD_START_ROUTINE)pRemoteFunction,
                                              pRemoteParameter, // 统一使用 pRemoteParameter
                                              0, NULL);

    if (!hRemoteThread) {
        DWORD error = GetLastError();
        emit errorOccurred(QString("无法创建远程线程调用函数 %1, 错误代码: %2").arg(functionName).arg(error));
        if (pRemoteParameter) VirtualFreeEx(hProcess, pRemoteParameter, 0, MEM_RELEASE); // 别忘了清理
        CloseHandle(hProcess);
        return false;
    }

    // --- 5. 等待并获取结果 ---
    DWORD waitResult = WaitForSingleObject(hRemoteThread, 5000);
    if (waitResult == WAIT_TIMEOUT) {
        emit errorOccurred(QString("远程线程调用超时: %1").arg(functionName));
        TerminateThread(hRemoteThread, 0);
        // ... 清理资源 ...
        if (pRemoteParameter) VirtualFreeEx(hProcess, pRemoteParameter, 0, MEM_RELEASE);
        CloseHandle(hRemoteThread);
        CloseHandle(hProcess);
        return false;
    }

    DWORD exitCode = 0;
    GetExitCodeThread(hRemoteThread, &exitCode);

    // --- 6. 清理所有资源 ---
    if (pRemoteParameter) {
        VirtualFreeEx(hProcess, pRemoteParameter, 0, MEM_RELEASE);
    }
    CloseHandle(hRemoteThread);
    CloseHandle(hProcess);

    bool success = (exitCode != 0);
    qDebug() << QString("远程调用 %1: 退出码=%2, 成功=%3")
                    .arg(functionName).arg(exitCode).arg(success);

    FreeLibrary(hLocalModule);

    return success;
}

FARPROC HookManager::getFunctionAddress(const QString &dllPath, const QString &functionName)
{
    HMODULE hModule = LoadLibraryExW(dllPath.toStdWString().c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);

    if (!hModule) {
        DWORD error = GetLastError();
        qDebug() << "无法以数据模式加载 DLL:" << dllPath << "错误代码:" << error;
        return nullptr;
    }

    // 获取函数地址
    FARPROC pFunction = GetProcAddress(hModule, functionName.toStdString().c_str());

    if (!pFunction) {
        QString functionNameW = functionName + "W";
        pFunction = GetProcAddress(hModule, functionNameW.toStdString().c_str());
    }

    return pFunction;
}

// ==================== 处理远程钩子相关的函数 ====================

bool HookManager::initialize(const QString &processName)
{
    // 检查 DLL 是否已经处于运行或安装状态，如果是，通常不需要重复 initialize
    HookState status = getHookStatus(processName);
    if (status != HookState::IDLE) {
        qDebug() << QString("进程 %1 当前状态为 %2，跳过 initialize")
                        .arg(processName)
                        .arg(hookStatusToString(status));
        return true;
    }

    qDebug() << "正在初始化远程 DLL -> 进程:" << processName;

    // 调用 DLL 导出的 initialize 函数
    bool result = callHookFunction(processName, "initialize");

    if (result) {
        qDebug() << "✅ 远程 DLL 初始化成功";
    } else {
        qDebug() << "❌ 远程 DLL 初始化失败";
    }

    return result;
}

bool HookManager::initialize(DWORD processId)
{
    HookState status = getHookStatus(processId);
    if (status != HookState::IDLE) {
        qDebug() << QString("进程ID %1 当前状态为 %2，跳过 initialize")
                        .arg(processId)
                        .arg(hookStatusToString(status));
        return true;
    }

    qDebug() << "正在初始化远程 DLL -> 进程ID:" << processId;

    bool result = callHookFunction(processId, "initialize");

    if (result) {
        qDebug() << "✅ 远程 DLL 初始化成功";
    } else {
        qDebug() << "❌ 远程 DLL 初始化失败";
    }

    return result;
}

bool HookManager::installAllHooks(const QString &processName)
{
    if (isInstalledOrInstalling(processName)) {
        qDebug() << "拦截：进程" << processName << "钩子已存在或正在安装中，取消重复调用。";
        return true;
    }

    qDebug() << "执行安装钩子到进程:" << processName;
    return callHookFunction(processName, "installAllHooks");
}

bool HookManager::installAllHooks(DWORD processId)
{
    if (isInstalledOrInstalling(processId)) {
        qDebug() << "拦截：进程ID" << processId << "钩子已存在或正在安装中，取消重复调用。";
        return true;
    }

    qDebug() << "执行安装钩子到进程ID:" << processId;
    return callHookFunction(processId, "installAllHooks");
}

bool HookManager::uninstallAllHooks(const QString &processName)
{
    qDebug() << "卸载钩子从进程:" << processName;
    return callHookFunction(processName, "uninstallAllHooks");
}

bool HookManager::uninstallAllHooks(DWORD processId)
{
    qDebug() << "卸载钩子从进程ID:" << processId;
    return callHookFunction(processId, "uninstallAllHooks");
}

HookState HookManager::getHookStatus(const QString &processName)
{
    return static_cast<HookState>(callHookFunction(processName, "getHookStatus"));
}

HookState HookManager::getHookStatus(DWORD processId)
{
    return static_cast<HookState>(callHookFunction(processId, "getHookStatus"));
}

bool HookManager::isInstalledOrInstalling(const QString &processName)
{
    HookState hookStatus = getHookStatus(processName);

    qDebug() << QString("进程 [%1] 当前钩子状态: %2")
                    .arg(processName)
                    .arg(hookStatusToString(hookStatus));

    if (hookStatus == HookState::INSTALLING || hookStatus == HookState::INSTALLED) {
        return true;
    }
    return false;
}

bool HookManager::isInstalledOrInstalling(DWORD processId)
{
    HookState hookStatus = getHookStatus(processId);

    qDebug() << QString("进程ID [%1] 当前钩子状态: %2")
                    .arg(processId)
                    .arg(hookStatusToString(hookStatus));

    if (hookStatus == HookState::INSTALLING || hookStatus == HookState::INSTALLED) {
        return true;
    }
    return false;
}

QString HookManager::hookStatusToString(HookState status)
{
    switch (status) {
    case HookState::IDLE:         return "未安装 (IDLE)";
    case HookState::INSTALLING:   return "正在安装 (INSTALLING)";
    case HookState::INSTALLED:    return "已安装 (INSTALLED)";
    case HookState::UNINSTALLING: return "正在卸载 (UNINSTALLING)";
    default:                       return QString("未知状态 (%1)").arg(static_cast<int>(status));
    }
}

// ==================== 远程函数导出信息相关方法 ====================

QVector<ExportFunctionInfo> HookManager::getRemoteExportFunctions(const QString &processName, const QString &dllName)
{
    DWORD processId = ProcessHelper::instance().getProcessIdByName(processName);
    if (processId == 0) {
        emit errorOccurred(QString("未找到进程: %1").arg(processName));
        return QVector<ExportFunctionInfo>();
    }

    return getRemoteExportFunctions(processId, dllName);
}

QVector<ExportFunctionInfo> HookManager::getRemoteExportFunctions(DWORD processId, const QString &dllName)
{
    QVector<ExportFunctionInfo> functions;

    if (!ProcessHelper::instance().openProcessByPid(processId)) {
        emit errorOccurred("无法打开指定进程");
        return functions;
    }

    QVariantMap moduleInfo = ProcessHelper::instance().getModuleInfo(dllName);
    if (!moduleInfo["isValid"].toBool()) {
        emit errorOccurred(QString("无法在目标进程中找到模块: %1").arg(dllName));
        return functions;
    }

    quintptr baseAddress = moduleInfo["baseAddress"].toULongLong();
    HMODULE hRemoteModule = reinterpret_cast<HMODULE>(baseAddress);

    HANDLE hProcess = ProcessHelper::instance().getProcessHandle(processId, PROCESS_VM_READ);
    if (!hProcess) {
        emit errorOccurred("无法以读取权限打开进程");
        return functions;
    }

    functions = parseRemoteExportTable(hProcess, hRemoteModule);
    CloseHandle(hProcess);

    qDebug() << QString("从进程 %1 的模块 %2 中获取到 %3 个导出函数")
                    .arg(processId).arg(dllName).arg(functions.size());

    return functions;
}

ExportFunctionInfo HookManager::getRemoteFunctionInfo(const QString &processName, const QString &dllName, const QString &functionName)
{
    DWORD processId = ProcessHelper::instance().getProcessIdByName(processName);
    if (processId == 0) {
        emit errorOccurred(QString("未找到进程: %1").arg(processName));
        return ExportFunctionInfo();
    }

    return getRemoteFunctionInfo(processId, dllName, functionName);
}

ExportFunctionInfo HookManager::getRemoteFunctionInfo(DWORD processId, const QString &dllName, const QString &functionName)
{
    QVector<ExportFunctionInfo> functions = getRemoteExportFunctions(processId, dllName);

    for (const ExportFunctionInfo &func : qAsConst(functions)) {
        if (func.name.compare(functionName, Qt::CaseInsensitive) == 0) {
            return func;
        }
    }

    // 如果没有找到名称匹配的函数，尝试查找序号
    bool ok;
    DWORD ordinal = functionName.toUInt(&ok);
    if (ok) {
        for (const ExportFunctionInfo &func : qAsConst(functions)) {
            if (func.ordinal == ordinal) {
                return func;
            }
        }
    }

    qDebug() << QString("在模块 %1 中未找到函数: %2").arg(dllName, functionName);
    return ExportFunctionInfo();
}

QVector<ExportFunctionInfo> HookManager::parseRemoteExportTable(HANDLE hProcess, HMODULE hRemoteModule)
{
    QVector<ExportFunctionInfo> functions;

    // 读取DOS头
    IMAGE_DOS_HEADER dosHeader;
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, hRemoteModule, &dosHeader, sizeof(dosHeader))) {
        qDebug() << "无法读取DOS头";
        return functions;
    }

    // 检查DOS签名
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) {
        qDebug() << "无效的DOS签名";
        return functions;
    }

    // 读取NT头
    IMAGE_NT_HEADERS ntHeaders;
    LPCVOID ntHeaderAddr = (LPCVOID)((DWORD)hRemoteModule + dosHeader.e_lfanew);
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, ntHeaderAddr, &ntHeaders, sizeof(ntHeaders))) {
        qDebug() << "无法读取NT头";
        return functions;
    }

    // 检查PE签名
    if (ntHeaders.Signature != IMAGE_NT_SIGNATURE) {
        qDebug() << "无效的PE签名";
        return functions;
    }

    // 获取导出表RVA和大小
    DWORD exportRva = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    DWORD exportSize = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

    if (exportRva == 0 || exportSize == 0) {
        qDebug() << "模块没有导出表";
        return functions;
    }

    // 读取导出表
    IMAGE_EXPORT_DIRECTORY exportDir;
    LPCVOID exportDirAddr = (LPCVOID)((DWORD)hRemoteModule + exportRva);
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, exportDirAddr, &exportDir, sizeof(exportDir))) {
        qDebug() << "无法读取导出目录";
        return functions;
    }

    // 计算各种数组的大小
    DWORD numberOfFunctions = exportDir.NumberOfFunctions;
    DWORD numberOfNames = exportDir.NumberOfNames;

    if (numberOfFunctions == 0) {
        qDebug() << "导出表中没有函数";
        return functions;
    }

    // 读取函数地址数组
    QVector<DWORD> functionAddresses(numberOfFunctions);
    LPCVOID functionAddrArray = (LPCVOID)((DWORD)hRemoteModule + exportDir.AddressOfFunctions);
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, functionAddrArray, functionAddresses.data(), numberOfFunctions * sizeof(DWORD))) {
        qDebug() << "无法读取函数地址数组";
        return functions;
    }

    // 读取函数名称数组
    QVector<DWORD> functionNamePointers(numberOfNames);
    LPCVOID namePtrArray = (LPCVOID)((DWORD)hRemoteModule + exportDir.AddressOfNames);
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, namePtrArray, functionNamePointers.data(), numberOfNames * sizeof(DWORD))) {
        qDebug() << "无法读取函数名称指针数组";
        return functions;
    }

    // 读取函数序号数组
    QVector<WORD> functionOrdinals(numberOfNames);
    LPCVOID ordinalArray = (LPCVOID)((DWORD)hRemoteModule + exportDir.AddressOfNameOrdinals);
    if (!ProcessHelper::instance().readRemoteMemory(hProcess, ordinalArray, functionOrdinals.data(), numberOfNames * sizeof(WORD))) {
        qDebug() << "无法读取函数序号数组";
        return functions;
    }

    // 构建导出函数信息
    for (DWORD i = 0; i < numberOfNames; i++) {
        ExportFunctionInfo info;

        // 读取函数名
        char functionName[256] = {0};
        LPCVOID nameAddr = (LPCVOID)((DWORD)hRemoteModule + functionNamePointers[i]);
        if (ProcessHelper::instance().readRemoteMemory(hProcess, nameAddr, functionName, sizeof(functionName) - 1)) {
            info.name = QString::fromLocal8Bit(functionName).trimmed();
        }

        // 设置序号信息
        info.ordinal = exportDir.Base + functionOrdinals[i];
        info.ordinalName = QString("#%1").arg(info.ordinal);
        info.isByName = true;

        // 获取函数RVA
        if (functionOrdinals[i] < numberOfFunctions) {
            info.rva = functionAddresses[functionOrdinals[i]];
        }

        functions.append(info);
    }

    // 添加仅通过序号导出的函数
    for (DWORD i = 0; i < numberOfFunctions; i++) {
        // 跳过空函数
        if (functionAddresses[i] == 0) {
            continue;
        }

        // 检查是否已经通过名称添加过
        bool alreadyAdded = false;
        for (const ExportFunctionInfo &func : qAsConst(functions)) {
            if (func.ordinal == exportDir.Base + i) {
                alreadyAdded = true;
                break;
            }
        }

        if (!alreadyAdded) {
            ExportFunctionInfo info;
            info.name = ""; // 没有名称
            info.ordinal = exportDir.Base + i;
            info.ordinalName = QString("#%1").arg(info.ordinal);
            info.rva = functionAddresses[i];
            info.isByName = false;

            functions.append(info);
        }
    }

    return functions;
}
