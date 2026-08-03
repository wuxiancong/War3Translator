#include "../managers/HookManager.h"
#include "../helpers/DebugHelper.h"
#include "../SingleApplication.h"
#include "ProcessHelper.h"
#include <QCoreApplication>
#include <QTextCodec>
#include <QFileInfo>
#include <QProcess>
#include <QTimer>
#include <QDebug>

#include <tlhelp32.h>
#include <psapi.h>

// 初始化静态成员变量
QMutex ProcessHelper::m_mutex;
QScopedPointer<ProcessHelper> ProcessHelper::s_instance;

ProcessHelper::ProcessHelper(QObject *parent)
    : QObject(parent)
    , m_currentPid(0)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("GBK"));
    if (enableDebugPrivilege()) {
        qDebug() << "成功启用 SeDebugPrivilege 调试权限。";
    } else {
        qDebug() << "警告: 启用 SeDebugPrivilege 调试权限失败！这很可能是因为没有以管理员身份运行。调试模式启动可能会失败。";
    }
}

ProcessHelper::~ProcessHelper()
{
    closeCurrentProcess();
    qDebug() << "ProcessHelper单例已销毁";
}

ProcessHelper &ProcessHelper::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&m_mutex);
        if (!s_instance) {
            s_instance.reset(new ProcessHelper());
        }
    }
    return *s_instance;
}

void ProcessHelper::restartApplication()
{
    DebugHelper::recordRestartEvent("RESTART", "准备发起重启指令...");

    // 1. 获取当前 app 实例并强制释放服务器
    if (auto *app = qobject_cast<SingleApplication*>(qApp)) {
        qDebug() << "释放 LocalServer 资源...";
        app->closeInstance();
    }

    QString program = QCoreApplication::applicationFilePath();
    QStringList arguments = QCoreApplication::arguments();
    if (!arguments.isEmpty()) arguments.removeFirst();

    // 清理可能存在的旧重启标志，防止无限叠加
    arguments.removeAll("--restarting");
    arguments << "--restarting" << QString::number(QCoreApplication::applicationPid());

    // 2. 启动新进程
    bool success = QProcess::startDetached(program, arguments, QCoreApplication::applicationDirPath());

    if (success) {
        DebugHelper::recordRestartEvent("RESTART", "新进程已分离启动。");
        // 3. 立即强制退出，不给旧进程干扰新进程的机会
        ::exit(0);
    } else {
        DebugHelper::recordRestartEvent("RESTART", "错误：QProcess 无法启动新实例。");
    }
}

bool ProcessHelper::enableDebugPrivilege()
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    ScopeHandle tokenHandle(hToken);

    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        return false;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    return result && GetLastError() == ERROR_SUCCESS;
}

DWORD ProcessHelper::getProcessIdByName(const QString &processName)
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hSnapshot, &pe)) {
        return 0;
    }

    do {
        QString currentProcess = QString::fromWCharArray(pe.szExeFile);

        if (currentProcess.compare(processName, Qt::CaseInsensitive) == 0) {
            return pe.th32ProcessID;
        }
    } while (Process32NextW(hSnapshot, &pe));

    return 0;
}

QString ProcessHelper::getProcessNameById(DWORD processId)
{
    // 方法1：使用进程快照（最可靠）
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        ScopeHandle snapshotHandle(hSnapshot);

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == processId) {
                    // 使用 fromLocal8Bit 转换进程名
                    return QString::fromLocal8Bit(pe.szExeFile).trimmed();
                }
            } while (Process32Next(hSnapshot, &pe));
        }
    }

    // 方法2：使用 QueryFullProcessImageName（备用方法）
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess) {
        wchar_t processPath[MAX_PATH];
        DWORD pathSize = MAX_PATH;

        if (QueryFullProcessImageNameW(hProcess, 0, processPath, &pathSize)) {
            CloseHandle(hProcess);
            QFileInfo fileInfo(QString::fromWCharArray(processPath));
            return fileInfo.fileName();
        }
        CloseHandle(hProcess);
    }

    // 方法3：使用 GetModuleFileNameEx（另一个备用方法）
    if (hProcess) {
        wchar_t processPath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
            CloseHandle(hProcess);
            QFileInfo fileInfo(QString::fromWCharArray(processPath));
            return fileInfo.fileName();
        }
        CloseHandle(hProcess);
    }

    return QString(); // 返回空字符串表示未找到
}

bool ProcessHelper::isProcessRunning(const QString &processName)
{
    return getProcessIdByName(processName) != 0;
}

HANDLE ProcessHelper::getProcessHandle(DWORD pid, DWORD desiredAccess)
{
    return OpenProcess(desiredAccess, FALSE, pid);
}

bool ProcessHelper::openProcess(const QString &processName)
{
    closeCurrentProcess();

    DWORD pid = getProcessIdByName(processName);
    if (pid == 0) {
        return false;
    }

    HANDLE hProcess = getProcessHandle(pid);
    if (!hProcess) {
        return false;
    }

    m_processHandle.reset(hProcess);
    m_currentPid = pid;
    m_currentProcessName = processName;

    return true;
}

bool ProcessHelper::openProcessByPid(DWORD pid)
{
    closeCurrentProcess();

    HANDLE hProcess = getProcessHandle(pid);
    if (!hProcess) {
        return false;
    }

    // 获取进程名
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        ScopeHandle snapshotHandle(hSnapshot);

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe)) {
            do {
                if (pe.th32ProcessID == pid) {
                    // 修复：使用 fromLocal8Bit 替代 fromWCharArray
                    m_currentProcessName = QString::fromLocal8Bit(pe.szExeFile).trimmed();
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }
    }

    m_processHandle.reset(hProcess);
    m_currentPid = pid;

    return true;
}

ModuleInfo ProcessHelper::getModuleInfoInternal(const QString &moduleName)
{
    ModuleInfo info;

    if (!m_processHandle.isValid()) {
        return info;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_currentPid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return info;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hSnapshot, &me)) {
        return info;
    }

    do {
        // 修复：使用 fromLocal8Bit 替代 fromWCharArray
        QString currentModule = QString::fromLocal8Bit(me.szModule).trimmed();
        if (currentModule.compare(moduleName, Qt::CaseInsensitive) == 0) {
            info.name = currentModule;
            info.baseAddress = reinterpret_cast<quintptr>(me.modBaseAddr);
            info.size = me.modBaseSize;
            // 修复：使用 fromLocal8Bit 替代 fromWCharArray
            info.path = QString::fromLocal8Bit(me.szExePath).trimmed();
            break;
        }
    } while (Module32Next(hSnapshot, &me));

    return info;
}

QVector<ModuleInfo> ProcessHelper::getAllModulesInternal()
{
    QVector<ModuleInfo> modules;

    if (!m_processHandle.isValid()) {
        return modules;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_currentPid);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return modules;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    MODULEENTRY32 me;
    me.dwSize = sizeof(MODULEENTRY32);

    if (!Module32First(hSnapshot, &me)) {
        return modules;
    }

    do {
        ModuleInfo info;
        // 修复：使用 fromLocal8Bit 替代 fromWCharArray
        info.name = QString::fromLocal8Bit(me.szModule).trimmed();
        info.baseAddress = reinterpret_cast<quintptr>(me.modBaseAddr);
        info.size = me.modBaseSize;
        // 修复：使用 fromLocal8Bit 替代 fromWCharArray
        info.path = QString::fromLocal8Bit(me.szExePath).trimmed();
        modules.append(info);
    } while (Module32Next(hSnapshot, &me));

    return modules;
}

// 主要方法：获取模块基地址
quintptr ProcessHelper::getModuleBaseAddress(const QString &moduleName)
{
    ModuleInfo info = getModuleInfoInternal(moduleName);
    return info.baseAddress;
}

QVariantMap ProcessHelper::getModuleInfo(const QString &moduleName)
{
    ModuleInfo info = getModuleInfoInternal(moduleName);

    QVariantMap map;
    map["name"] = info.name;
    map["baseAddress"] = info.baseAddress;
    map["size"] = info.size;
    map["path"] = info.path;
    map["isValid"] = !info.name.isEmpty();

    return map;
}

QVector<QVariantMap> ProcessHelper::getAllModules()
{
    QVector<ModuleInfo> modules = getAllModulesInternal();
    QVector<QVariantMap> result;

    for (const ModuleInfo &info : qAsConst(modules)) {
        QVariantMap map;
        map["name"] = info.name;
        map["baseAddress"] = info.baseAddress;
        map["size"] = info.size;
        map["path"] = info.path;
        result.append(map);
    }

    return result;
}

QVector<QString> ProcessHelper::getModuleList()
{
    QVector<ModuleInfo> modules = getAllModulesInternal();
    QVector<QString> result;

    for (const ModuleInfo &info : qAsConst(modules)) {
        result.append(info.name);
    }

    return result;
}

quintptr ProcessHelper::getMainThreadHandle()
{
    if (!m_processHandle.isValid()) {
        return 0;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hSnapshot, &te)) {
        return 0;
    }

    DWORD mainThreadId = 0;
    quint64 minCreateTime = ULLONG_MAX;

    do {
        if (te.th32OwnerProcessID == m_currentPid) {
            HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
            if (hThread) {
                ScopeHandle threadHandle(hThread);

                FILETIME createTime, exitTime, kernelTime, userTime;
                if (GetThreadTimes(hThread, &createTime, &exitTime, &kernelTime, &userTime)) {
                    quint64 createTime64 = (static_cast<quint64>(createTime.dwHighDateTime) << 32) | createTime.dwLowDateTime;
                    if (createTime64 < minCreateTime) {
                        minCreateTime = createTime64;
                        mainThreadId = te.th32ThreadID;
                    }
                }
            }
        }
    } while (Thread32Next(hSnapshot, &te));

    if (mainThreadId != 0) {
        HANDLE hMainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, mainThreadId);
        if (hMainThread) {
            return reinterpret_cast<quintptr>(hMainThread);
        }
    }

    return 0;
}

QVector<quintptr> ProcessHelper::getAllThreadHandles()
{
    QVector<quintptr> threadHandles;

    if (!m_processHandle.isValid()) {
        return threadHandles;
    }

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return threadHandles;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (!Thread32First(hSnapshot, &te)) {
        return threadHandles;
    }

    do {
        if (te.th32OwnerProcessID == m_currentPid) {
            HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
            if (hThread) {
                threadHandles.append(reinterpret_cast<quintptr>(hThread));
            }
        }
    } while (Thread32Next(hSnapshot, &te));

    return threadHandles;
}

void ProcessHelper::closeCurrentProcess()
{
    if (m_processHandle.isValid()) {
        m_processHandle.reset();
        m_currentPid = 0;
        m_currentProcessName.clear();
    }
}

QVector<QVariantMap> ProcessHelper::getAllProcesses()
{
    QVector<QVariantMap> processes;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return processes;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe)) {
        return processes;
    }

    do {
        QVariantMap process;
        // 修复：使用 QVariant::fromValue 明确指定类型
        process["pid"] = QVariant::fromValue<DWORD>(pe.th32ProcessID);
        // 修复：使用 fromLocal8Bit 替代 fromWCharArray
        process["name"] = QString::fromLocal8Bit(pe.szExeFile).trimmed();
        process["parentPid"] = QVariant::fromValue<DWORD>(pe.th32ParentProcessID);
        process["threadCount"] = QVariant::fromValue<DWORD>(pe.cntThreads);
        process["priority"] = QVariant::fromValue<LONG>(pe.pcPriClassBase);

        processes.append(process);
    } while (Process32Next(hSnapshot, &pe));

    return processes;
}

bool ProcessHelper::readRemoteMemory(HANDLE hProcess, LPCVOID remoteAddress, LPVOID localBuffer, SIZE_T size)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProcess, remoteAddress, localBuffer, size, &bytesRead) && (bytesRead == size);
}

PNtQueryInformationProcess ProcessHelper::getNtQueryInformationProcess()
{
    if(g_pNtQueryInformationProcess == nullptr)
    {
        HMODULE hNtdll = LoadLibrary("ntdll.dll");
        if(hNtdll != nullptr)
        {
            g_pNtQueryInformationProcess = (PNtQueryInformationProcess)GetProcAddress(
                hNtdll,
                "NtQueryInformationProcess");
            if(g_pNtQueryInformationProcess == nullptr)
            {
                qDebug() << "获取函数失败！[NtQueryInformationProcess]";
            }
        }else{
            qDebug() << "加载模块失败！[hNtdll]";
        }
    }
    return g_pNtQueryInformationProcess;
}

DWORD ProcessHelper::getProcessBaseAddress(HANDLE hThread, HANDLE hProcess)
{
    DWORD baseAddress;
    baseAddress = getProcessBaseAddressByWinApi(hProcess);
    if (baseAddress != 0xFFFFFFFF) return baseAddress;
    return getProcessBaseAddressByRegister(hThread, hProcess);
}

DWORD ProcessHelper::getProcessBaseAddressByWinApi(HANDLE hProcess)
{
    DWORD dwSize = 0;
    HANDLE hHeap = 0;
    DWORD pebBaseAddress;
    DWORD dwBytesRead = 0;
    DWORD dwSizeNeeded = 0;
    DWORD dwBufferSize = 0;
    WCHAR *pwszBuffer = NULL;
    DWORD baseAddress = 0xFFFFFFFF;
    PPROCESS_BASIC_INFORMATION pbi = NULL;
    if (hProcess == INVALID_HANDLE_VALUE) {
        return baseAddress;
    }
    hHeap = GetProcessHeap();
    dwSize = sizeof(PPROCESS_BASIC_INFORMATION);
    pbi = (PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSize);
    if (!pbi) {
        return baseAddress;
    }
    NTSTATUS dwStatus = g_pNtQueryInformationProcess(hProcess, ProcessBasicInformation, pbi, dwSize, &dwSizeNeeded);
    if (dwStatus >= 0 && dwSize < dwSizeNeeded)
    {
        if (pbi) HeapFree(hHeap, 0, pbi);
        pbi = (PPROCESS_BASIC_INFORMATION)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, dwSizeNeeded);
        if (!pbi) {
            return baseAddress;
        }
        dwStatus = g_pNtQueryInformationProcess(hProcess, ProcessBasicInformation, pbi, dwSizeNeeded, &dwSizeNeeded);
    }
    if (dwStatus >= 0)
    {
        pebBaseAddress = (DWORD)pbi->PebBaseAddress;
        if (pbi)HeapFree(hHeap, 0, pbi);
        SIZE_T numread = 0;
        char buf[4];
        if (!ReadProcessMemory(hProcess, (LPCVOID)(pebBaseAddress + 8), buf, 4, &numread)) {
            return 0xFFFFFFFF;
        }
        baseAddress = *(DWORD*)buf;
    }
    return baseAddress;
}

DWORD ProcessHelper::getProcessBaseAddressByRegister(HANDLE hThread, HANDLE hProcess)
{
    DWORD ebx;
    char buf[4];
    CONTEXT ctx;
    SIZE_T numread = 0;
    DWORD baseAddress = 0xFFFFFFFF;
    ctx.ContextFlags = CONTEXT_ALL;
    GetThreadContext(hThread, &ctx);
    ebx = ctx.Ebx;
    if (!ReadProcessMemory(hProcess, (LPCVOID)(ebx + 8), buf, 4, &numread)) {
        return 0xFFFFFFFF;
    }
    baseAddress = *(DWORD*)buf;
    return baseAddress;
}

// 获取QProcess的基地址和进程信息
QVector<QVariantMap> ProcessHelper::getProcessInfoFromQProcess(QProcess *process)
{
    QVector<QVariantMap> result;

    if (!process || process->state() != QProcess::Running) {
        qDebug() << "QProcess未运行或无效";
        return result;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        qDebug() << "获取QProcess进程ID失败";
        return result;
    }

    // 使用现有的openProcessByPid方法打开进程
    if (!openProcessByPid(static_cast<DWORD>(pid))) {
        qDebug() << "通过PID打开QProcess失败:" << pid;
        return result;
    }

    // 获取进程的所有模块信息
    QVector<ModuleInfo> modules = getAllModulesInternal();

    for (const ModuleInfo &module : qAsConst(modules)) {
        QVariantMap moduleInfo;
        moduleInfo["name"] = module.name;
        moduleInfo["baseAddress"] = QString("0x%1").arg(module.baseAddress, 0, 16);
        moduleInfo["decimalAddress"] = module.baseAddress;
        moduleInfo["size"] = module.size;
        moduleInfo["path"] = module.path;

        // 标记主模块（通常是exe文件）
        if (module.name.compare(m_currentProcessName, Qt::CaseInsensitive) == 0) {
            moduleInfo["isMainModule"] = true;
        } else {
            moduleInfo["isMainModule"] = false;
        }

        result.append(moduleInfo);
    }

    qDebug() << "成功获取进程模块信息，共" << result.size() << "个模块";
    return result;
}

QVariantMap ProcessHelper::getQProcessBaseAddressWithModuleName(QProcess *process, const QString &moduleName)
{
    QVariantMap result;

    if (!process || process->state() != QProcess::Running) {
        result["error"] = "QProcess未运行或无效";
        qDebug() << "获取基地址失败: QProcess未运行";
        return result;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        result["error"] = "获取QProcess进程ID失败";
        qDebug() << "获取基地址失败: 无效的进程ID";
        return result;
    }

    // 打开进程
    if (!openProcessByPid(static_cast<DWORD>(pid))) {
        QString errorMsg = QString("打开进程失败，PID: %1").arg(pid);
        result["error"] = errorMsg;
        qDebug() << "获取基地址失败:" << errorMsg;
        return result;
    }

    QString targetModule = moduleName.isEmpty() ? m_currentProcessName : moduleName;

    // 获取指定模块的基地址
    qintptr baseAddress = getModuleBaseAddress(targetModule);

    if (baseAddress == 0) {
        QString errorMsg = QString("获取模块基地址失败: %1").arg(targetModule);
        result["error"] = errorMsg;
        qDebug() << errorMsg;
        return result;
    }

    result["success"] = true;
    result["processId"] = pid;
    result["processName"] = m_currentProcessName;
    result["moduleName"] = targetModule;
    result["baseAddress"] = QString("0x%1").arg(baseAddress, 0, 16);
    result["decimalAddress"] = baseAddress;

    qDebug() << QString("成功获取模块基地址: %1 -> 0x%2")
                    .arg(targetModule)
                    .arg(baseAddress, 0, 16);

    return result;
}

QVariantMap ProcessHelper::getQProcessDetailedInfo(QProcess *process)
{
    QVariantMap result;

    if (!process || process->state() != QProcess::Running) {
        result["error"] = "QProcess未运行或无效";
        qDebug() << "获取详细进程信息失败: QProcess未运行";
        return result;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        result["error"] = "获取QProcess进程ID失败";
        qDebug() << "获取详细进程信息失败: 无效的进程ID";
        return result;
    }

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, static_cast<DWORD>(pid));
    if (!hProcess) {
        QString errorMsg = QString("打开进程句柄失败，PID: %1").arg(pid);
        result["error"] = errorMsg;
        qDebug() << "获取详细进程信息失败:" << errorMsg;
        return result;
    }

    ScopeHandle processHandle(hProcess);

    // 获取进程基本信息
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        result["workingSetSize"] = static_cast<qint64>(pmc.WorkingSetSize);
        result["pagefileUsage"] = static_cast<qint64>(pmc.PagefileUsage);
        result["peakWorkingSetSize"] = static_cast<qint64>(pmc.PeakWorkingSetSize);
        result["peakPagefileUsage"] = static_cast<qint64>(pmc.PeakPagefileUsage);
        qDebug() << "成功获取进程内存信息";
    } else {
        qDebug() << "获取进程内存信息失败";
    }

    // 获取进程时间信息
    FILETIME createTime, exitTime, kernelTime, userTime;
    if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
        // 转换为64位值便于处理
        ULARGE_INTEGER createTime64;
        createTime64.LowPart = createTime.dwLowDateTime;
        createTime64.HighPart = createTime.dwHighDateTime;

        result["createTime"] = static_cast<qint64>(createTime64.QuadPart);
        qDebug() << "成功获取进程时间信息";
    } else {
        qDebug() << "获取进程时间信息失败";
    }

    // 获取优先级信息
    DWORD priority = GetPriorityClass(hProcess);
    result["priorityClass"] = static_cast<uint>(priority);

    // 获取所有模块信息
    QVector<QVariantMap> modules = getProcessInfoFromQProcess(process);
    result["modules"] = QVariant::fromValue(modules);
    result["moduleCount"] = static_cast<int>(modules.size());

    result["success"] = true;
    result["processId"] = static_cast<qint64>(pid);
    result["processState"] = static_cast<int>(process->state());

    qDebug() << QString("成功获取进程详细信息，PID: %1，模块数量: %2")
                    .arg(pid)
                    .arg(modules.size());

    return result;
}

QString ProcessHelper::getQProcessMainModuleBaseAddress(QProcess *process)
{
    if (!process || process->state() != QProcess::Running) {
        qDebug() << "获取主模块基地址失败: 进程未运行";
        return "错误: 进程未运行";
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        qDebug() << "获取主模块基地址失败: 无效的进程ID";
        return "错误: 无效的进程ID";
    }

    if (!openProcessByPid(static_cast<DWORD>(pid))) {
        qDebug() << QString("获取主模块基地址失败: 无法打开进程 %1").arg(pid);
        return QString("错误: 无法打开进程 %1").arg(pid);
    }

    DWORD_PTR baseAddress = getModuleBaseAddress(m_currentProcessName);
    if (baseAddress == 0) {
        qDebug() << "获取主模块基地址失败: 无法获取基地址";
        return "错误: 无法获取基地址";
    }

    QString addressStr = QString("0x%1").arg(baseAddress, 0, 16);
    qDebug() << QString("成功获取主模块基地址: %1 -> %2")
                    .arg(m_currentProcessName, addressStr);

    return addressStr;
}

bool ProcessHelper::attachToQProcess(QProcess *process)
{
    if (!process || process->state() != QProcess::Running) {
        qDebug() << "附加到进程失败: QProcess未运行或无效";
        return false;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        qDebug() << "附加到进程失败: 无效的进程ID";
        return false;
    }

    bool success = openProcessByPid(static_cast<DWORD>(pid));
    if (success) {
        qDebug() << QString("成功附加到进程，PID: %1，进程名: %2")
                        .arg(pid)
                        .arg(m_currentProcessName);
    } else {
        qDebug() << QString("附加到进程失败，PID: %1").arg(pid);
    }

    return success;
}

QString ProcessHelper::getCurrentAttachedProcessInfo() const
{
    if (!m_processHandle.isValid()) {
        return "未附加到任何进程";
    }

    return QString("进程ID: %1, 进程名: %2").arg(QString::number(m_currentPid), m_currentProcessName);
}

// 获取进程状态信息
QVariantMap ProcessHelper::getQProcessStatus(QProcess *process)
{
    QVariantMap status;

    if (!process) {
        status["error"] = "QProcess指针为空";
        return status;
    }

    status["processId"] = static_cast<qint64>(process->processId());
    status["state"] = static_cast<int>(process->state());
    status["program"] = process->program();
    status["workingDirectory"] = process->workingDirectory();
    status["isAttached"] = (m_currentPid == static_cast<DWORD>(process->processId()));

    // 状态描述
    switch (process->state()) {
    case QProcess::NotRunning:
        status["stateDescription"] = "未运行";
        break;
    case QProcess::Starting:
        status["stateDescription"] = "启动中";
        break;
    case QProcess::Running:
        status["stateDescription"] = "运行中";
        break;
    default:
        status["stateDescription"] = "未知状态";
        break;
    }

    return status;
}

// 获取进程基地址
DWORD_PTR ProcessHelper::getQProcessBaseAddress(QProcess *process)
{
    if (!process || process->state() != QProcess::Running) {
        qDebug() << "获取基地址失败: 进程未运行";
        return 0;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        qDebug() << "获取基地址失败: 无效的进程ID";
        return 0;
    }

    if (!openProcessByPid(static_cast<DWORD>(pid))) {
        qDebug() << QString("获取基地址失败: 无法打开进程 %1").arg(pid);
        return 0;
    }

    DWORD_PTR baseAddress = getModuleBaseAddress(m_currentProcessName);
    if (baseAddress == 0) {
        qDebug() << "获取基地址失败: 无法获取基地址";
        return 0;
    }

    qDebug() << QString("成功获取基地址: 0x%1").arg(baseAddress, 0, 16);
    return baseAddress;
}

// 获取 PROCESS_INFORMATION 类型的进程信息
PROCESS_INFORMATION ProcessHelper::getQProcessInformation(QProcess *process)
{
    PROCESS_INFORMATION procInfo = {0};

    if (!process || process->state() != QProcess::Running) {
        qDebug() << "获取进程信息失败: QProcess未运行或无效";
        return procInfo;
    }

    qint64 pid = process->processId();
    if (pid == 0) {
        qDebug() << "获取进程信息失败: 无效的进程ID";
        return procInfo;
    }

    return getProcessInformationByPid(static_cast<DWORD>(pid));
}

// 通过进程ID获取 PROCESS_INFORMATION
PROCESS_INFORMATION ProcessHelper::getProcessInformationByPid(DWORD pid)
{
    PROCESS_INFORMATION procInfo = {0};

    if (pid == 0) {
        qDebug() << "获取进程信息失败: PID为0";
        return procInfo;
    }

    // 设置进程ID
    procInfo.dwProcessId = pid;

    // 打开进程句柄
    procInfo.hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!procInfo.hProcess) {
        qDebug() << QString("打开进程句柄失败，PID: %1，错误代码: %2").arg(pid).arg(GetLastError());
        // 继续尝试获取线程信息，即使进程句柄打开失败
    }

    // 获取主线程信息
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        ScopeHandle snapshotHandle(hSnapshot);

        THREADENTRY32 te;
        te.dwSize = sizeof(THREADENTRY32);

        DWORD mainThreadId = 0;
        quint64 minCreateTime = ULLONG_MAX;

        if (Thread32First(hSnapshot, &te)) {
            do {
                if (te.th32OwnerProcessID == pid) {
                    // 查找创建时间最早的线程（通常是主线程）
                    HANDLE hThread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
                    if (hThread) {
                        ScopeHandle threadHandle(hThread);

                        FILETIME createTime, exitTime, kernelTime, userTime;
                        if (GetThreadTimes(hThread, &createTime, &exitTime, &kernelTime, &userTime)) {
                            quint64 createTime64 = (static_cast<quint64>(createTime.dwHighDateTime) << 32) | createTime.dwLowDateTime;
                            if (createTime64 < minCreateTime) {
                                minCreateTime = createTime64;
                                mainThreadId = te.th32ThreadID;
                            }
                        }
                    }
                }
            } while (Thread32Next(hSnapshot, &te));
        }

        if (mainThreadId != 0) {
            procInfo.dwThreadId = mainThreadId;
            procInfo.hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, mainThreadId);
            if (!procInfo.hThread) {
                qDebug() << QString("打开线程句柄失败，线程ID: %1，错误代码: %2").arg(mainThreadId).arg(GetLastError());
            }
        }
    }

    qDebug() << QString("成功获取进程信息 - PID: %1, 主线程ID: %2, 进程句柄: %3, 线程句柄: %4")
                    .arg(procInfo.dwProcessId)
                    .arg(procInfo.dwThreadId)
                    .arg(reinterpret_cast<quintptr>(procInfo.hProcess))
                    .arg(reinterpret_cast<quintptr>(procInfo.hThread));

    return procInfo;
}

BOOL CALLBACK ProcessHelper::enumWindowsProcClose(HWND hwnd, LPARAM lParam)
{
    DWORD targetPid = static_cast<DWORD>(lParam);
    DWORD windowPid;
    GetWindowThreadProcessId(hwnd, &windowPid);

    if (windowPid == targetPid) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        qDebug() << "向窗口发送关闭消息，PID:" << targetPid;
    }
    return TRUE;
}

bool ProcessHelper::forceTerminateProcessEx(const QString &processName, bool forceIfHung)
{
    qDebug() << "增强版进程终止:" << processName;

    DWORD pid = getProcessIdByName(processName);
    if (pid == 0) {
        qDebug() << "未找到进程:" << processName;
        return false;
    }

    // 打开进程
    DWORD desiredAccess = PROCESS_TERMINATE;
    if (forceIfHung) {
        desiredAccess |= PROCESS_QUERY_INFORMATION;
    }

    HANDLE hProcess = OpenProcess(desiredAccess, FALSE, pid);
    if (!hProcess) {
        qDebug() << "打开进程失败，PID:" << pid;
        return false;
    }

    ScopeHandle processHandle(hProcess);

    // 如果要求强制终止无响应进程
    if (forceIfHung) {
        // 检查进程是否无响应
        DWORD exitCode;
        if (GetExitCodeProcess(hProcess, &exitCode)) {
            if (exitCode == STILL_ACTIVE) {
                // 进程仍在运行，尝试发送关闭消息（对于有界面的进程）
                EnumWindows(enumWindowsProcClose, static_cast<LPARAM>(pid));

                // 等待一段时间让进程正常关闭
                Sleep(1000);
            }
        }
    }

    // 强制终止进程
    if (TerminateProcess(hProcess, 0)) {
        qDebug() << "成功终止进程:" << processName << "PID:" << pid;

        // 等待进程退出
        WaitForSingleObject(hProcess, 3000);
        return true;
    } else {
        qDebug() << "终止进程失败:" << processName;
        return false;
    }
}

bool ProcessHelper::forceTerminateProcess(const QString &processName)
{
    qDebug() << "尝试强制关闭进程:" << processName;

    // 获取进程ID
    DWORD pid = getProcessIdByName(processName);
    if (pid == 0) {
        qDebug() << "未找到进程:" << processName;
        return false;
    }

    return forceTerminateProcessByPid(pid);
}

bool ProcessHelper::forceTerminateProcessByPid(DWORD pid)
{
    qDebug() << "尝试强制关闭进程，PID:" << pid;

    if (pid == 0) {
        qDebug() << "无效的进程ID";
        return false;
    }

    // 检查是否是当前进程自身
    if (pid == GetCurrentProcessId()) {
        qDebug() << "不能关闭自身进程";
        return false;
    }

    // 打开进程，获取 PROCESS_TERMINATE 权限
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) {
        DWORD error = GetLastError();
        qDebug() << "打开进程失败，PID:" << pid << "错误代码:" << error;
        return false;
    }

    ScopeHandle processHandle(hProcess);

    // 尝试正常终止进程
    if (TerminateProcess(hProcess, 0)) {
        qDebug() << "成功终止进程，PID:" << pid;

        // 等待进程完全退出（可选，超时时间2秒）
        DWORD waitResult = WaitForSingleObject(hProcess, 2000);
        if (waitResult == WAIT_OBJECT_0) {
            qDebug() << "进程已完全退出，PID:" << pid;
        } else {
            qDebug() << "进程终止但可能未完全退出，PID:" << pid;
        }

        return true;
    } else {
        DWORD error = GetLastError();
        qDebug() << "终止进程失败，PID:" << pid << "错误代码:" << error;
        return false;
    }
}

bool ProcessHelper::terminateCurrentProcess()
{
    if (!m_processHandle.isValid()) {
        qDebug() << "没有已打开的进程可终止";
        return false;
    }

    qDebug() << "尝试终止当前已打开的进程:" << m_currentProcessName << "PID:" << m_currentPid;

    if (TerminateProcess(m_processHandle.get(), 0)) {
        qDebug() << "成功终止当前进程:" << m_currentProcessName;

        // 等待进程退出
        DWORD waitResult = WaitForSingleObject(m_processHandle.get(), 2000);
        if (waitResult == WAIT_OBJECT_0) {
            qDebug() << "当前进程已完全退出";
        }

        // 清理资源
        closeCurrentProcess();
        return true;
    } else {
        DWORD error = GetLastError();
        qDebug() << "终止当前进程失败，错误代码:" << error;
        return false;
    }
}

QVector<DWORD> ProcessHelper::getAllProcessIdsByName(const QString &processName)
{
    QVector<DWORD> pids;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        qDebug() << "创建进程快照失败";
        return pids;
    }

    ScopeHandle snapshotHandle(hSnapshot);

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe)) {
        return pids;
    }

    DWORD currentPid = GetCurrentProcessId();

    do {
        QString currentProcess = QString::fromLocal8Bit(pe.szExeFile).trimmed();
        if (currentProcess.compare(processName, Qt::CaseInsensitive) == 0) {
            // 排除自身进程
            if (pe.th32ProcessID != currentPid) {
                pids.append(pe.th32ProcessID);
            }
        }
    } while (Process32Next(hSnapshot, &pe));

    qDebug() << "找到进程" << processName << "的实例数量:" << pids.size();
    return pids;
}

bool ProcessHelper::forceTerminateAllProcesses(const QString &processName)
{
    qDebug() << "尝试关闭所有同名进程:" << processName;

    QVector<DWORD> pids = getAllProcessIdsByName(processName);
    if (pids.isEmpty()) {
        qDebug() << "未找到进程:" << processName;
        return false;
    }

    return forceTerminateProcesses(pids);
}

bool ProcessHelper::forceTerminateAllProcessesExcludeSelf(const QString &processName)
{
    qDebug() << "尝试关闭所有同名进程（排除自身）:" << processName;

    QVector<DWORD> pids = getAllProcessIdsByName(processName);
    if (pids.isEmpty()) {
        qDebug() << "未找到进程:" << processName;
        return false;
    }

    return forceTerminateProcesses(pids);
}

bool ProcessHelper::forceTerminateProcesses(const QVector<DWORD> &pids)
{
    if (pids.isEmpty()) {
        return true; // 没有进程需要关闭，视为成功
    }

    qDebug() << "开始批量关闭" << pids.size() << "个进程";

    bool allSuccess = true;
    int successCount = 0;

    for (DWORD pid : pids) {
        if (forceTerminateProcessByPid(pid)) {
            successCount++;
            qDebug() << "成功关闭进程 PID:" << pid;
        } else {
            allSuccess = false;
            qDebug() << "关闭进程失败 PID:" << pid;
        }

        // 添加短暂延迟，避免系统资源冲突
        Sleep(100);
    }

    qDebug() << "批量关闭完成: 成功" << successCount << "/" << pids.size();
    return allSuccess;
}

BOOL CALLBACK ProcessHelper::enumWindowsProcFindPid(HWND hwnd, LPARAM lParam)
{
    struct FindPidData {
        DWORD pid;
        bool windowFound;
    };

    FindPidData *data = reinterpret_cast<FindPidData*>(lParam);
    DWORD windowPid;
    GetWindowThreadProcessId(hwnd, &windowPid);

    if (windowPid == data->pid) {
        if (PostMessage(hwnd, WM_CLOSE, 0, 0)) {
            data->windowFound = true;
        }
    }
    return TRUE;
}

bool ProcessHelper::smartTerminateProcesses(const QString &processName, int timeoutMs)
{
    qDebug() << "智能关闭进程:" << processName;

    QVector<DWORD> pids = getAllProcessIdsByName(processName);
    if (pids.isEmpty()) {
        qDebug() << "未找到进程:" << processName;
        return true;
    }

    // 第一阶段：尝试正常关闭（发送关闭消息）
    qDebug() << "第一阶段：尝试正常关闭";
    QVector<DWORD> remainingPids;

    for (DWORD pid : qAsConst(pids)) {
        // 修复：使用结构体而不是临时对象
        struct WindowCloseData {
            DWORD pid;
            bool windowFound;
        };

        WindowCloseData closeData{pid, false};

        // 查找该进程的所有窗口并发送关闭消息
        EnumWindows(enumWindowsProcFindPid, reinterpret_cast<LPARAM>(&closeData));

        if (!closeData.windowFound) {
            remainingPids.append(pid);
            qDebug() << "未找到窗口，将强制关闭 PID:" << pid;
        }
    }

    // 等待一段时间让进程正常关闭
    if (timeoutMs > 0) {
        qDebug() << "等待" << timeoutMs << "毫秒让进程正常关闭...";
        Sleep(static_cast<DWORD>(timeoutMs));
    }

    // 检查哪些进程已经退出
    QVector<DWORD> stillRunningPids;
    for (DWORD pid : qAsConst(pids)) {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (hProcess) {
            DWORD exitCode;
            if (GetExitCodeProcess(hProcess, &exitCode)) {
                if (exitCode == STILL_ACTIVE) {
                    stillRunningPids.append(pid);
                } else {
                    qDebug() << "进程已退出 PID:" << pid << "退出码:" << exitCode;
                }
            }
            CloseHandle(hProcess);
        } else {
            // 无法打开进程，可能已经退出
            qDebug() << "无法打开进程，可能已退出 PID:" << pid;
        }
    }

    // 第二阶段：强制关闭仍在运行的进程
    if (!stillRunningPids.isEmpty()) {
        qDebug() << "第二阶段：强制关闭仍在运行的" << stillRunningPids.size() << "个进程";
        return forceTerminateProcesses(stillRunningPids);
    }

    qDebug() << "所有进程已正常关闭";
    return true;
}

bool ProcessHelper::forceTerminateProcessTree(const QString &processName)
{
    qDebug() << "关闭进程树:" << processName;

    QVector<DWORD> pids = getAllProcessIdsByName(processName);
    if (pids.isEmpty()) {
        qDebug() << "未找到进程:" << processName;
        return false;
    }

    QVector<DWORD> allPidsToTerminate;

    // 获取所有需要关闭的进程（包括子进程）
    for (DWORD parentPid : qAsConst(pids)) {
        allPidsToTerminate.append(parentPid);

        // 查找子进程
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            ScopeHandle snapshotHandle(hSnapshot);

            PROCESSENTRY32 pe;
            pe.dwSize = sizeof(PROCESSENTRY32);

            if (Process32First(hSnapshot, &pe)) {
                do {
                    if (pe.th32ParentProcessID == parentPid) {
                        allPidsToTerminate.append(pe.th32ProcessID);
                        qDebug() << "找到子进程:" << QString::fromLocal8Bit(pe.szExeFile).trimmed() << "PID:" << pe.th32ProcessID;
                    }
                } while (Process32Next(hSnapshot, &pe));
            }
        }
    }

    // 去重
    std::sort(allPidsToTerminate.begin(), allPidsToTerminate.end());
    allPidsToTerminate.erase(std::unique(allPidsToTerminate.begin(), allPidsToTerminate.end()),
                             allPidsToTerminate.end());

    qDebug() << "需要关闭的进程数量:" << allPidsToTerminate.size() << "(包含子进程)";
    return forceTerminateProcesses(allPidsToTerminate);
}

DWORD ProcessHelper::findLatestWar3Process()
{
    qDebug() << "=== 开始详细查找 War3 进程 ===";

    DWORD latestPid = 0;
    ULONGLONG latestTime = 0;
    int processCount = 0;
    int totalProcesses = 0;

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        qDebug() << "创建进程快照失败，错误代码:" << error;
        return 0;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnapshot, &pe)) {
        do {
            totalProcesses++;

            // 改用 QString::fromWCharArray 处理进程名称，避免编码问题
            QString processName = QString::fromLocal8Bit(pe.szExeFile).trimmed();

            // 调试输出所有包含 "war3" 的进程
            if (processName.contains("war3", Qt::CaseInsensitive) ||
                processName.contains("warcraft", Qt::CaseInsensitive)) {
                qDebug() << "相关进程:" << processName << "PID:" << pe.th32ProcessID;
            }

            // 使用更宽松的比较方式
            if (processName.compare("War3.exe", Qt::CaseInsensitive) == 0) {
                processCount++;
                qDebug() << "!!! 发现 War3.exe 进程 !!! PID:" << pe.th32ProcessID;

                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    FILETIME createTime, exitTime, kernelTime, userTime;
                    if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
                        ULARGE_INTEGER uli;
                        uli.LowPart = createTime.dwLowDateTime;
                        uli.HighPart = createTime.dwHighDateTime;

                        qDebug() << "进程" << pe.th32ProcessID << "创建时间:" << uli.QuadPart;

                        if (uli.QuadPart > latestTime) {
                            latestTime = uli.QuadPart;
                            latestPid = pe.th32ProcessID;
                            qDebug() << "*** 更新最新进程 PID:" << latestPid;
                        }
                    } else {
                        DWORD error = GetLastError();
                        qDebug() << "获取进程时间失败，PID:" << pe.th32ProcessID << "错误代码:" << error;
                    }
                    CloseHandle(hProcess);
                } else {
                    DWORD error = GetLastError();
                    qDebug() << "打开进程失败，PID:" << pe.th32ProcessID << "错误代码:" << error;
                }
            }
        } while (Process32Next(hSnapshot, &pe));
    } else {
        DWORD error = GetLastError();
        qDebug() << "枚举进程失败，错误代码:" << error;
    }

    CloseHandle(hSnapshot);

    qDebug() << "=== 进程查找结束 ===";
    qDebug() << "系统总进程数:" << totalProcesses;
    qDebug() << "找到 War3.exe 进程数:" << processCount;
    qDebug() << "最终选择的 PID:" << latestPid;

    return latestPid;
}

bool ProcessHelper::injectHookDll(const QString &gamePath)
{
    DWORD war3Pid = findLatestWar3Process();
    if (war3Pid == 0) {
        qDebug() << "War3.exe 验证失败，Pid: (" << war3Pid << ") Path: " << gamePath;
        return false;
    }
    qDebug() << "War3.exe 验证成功，Pid: (" << war3Pid << ") Path: " << gamePath;
    if(!HookManager::instance().injectTranslatorDll(war3Pid)) {
        qDebug() << "translator.dll 模块注入失败";
        return false;
    }
    qDebug() << "translator.dll 模块注入成功";
    return true;
}
