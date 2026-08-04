#include "translator.h"
#include "../logger/war3loga.h"
#include <cassert>
#include <unordered_set>
#include <tlhelp32.h>
#include <algorithm>
#include <sstream>
#include <psapi.h>
#include <iomanip>
#include <set>

const wchar_t *TREE_ITEM = L"├── ";
const wchar_t *TREE_LAST = L"└── ";
const wchar_t *TREE_LINE = L"│   ";
const uint64_t CACHE_TTL_MS = 1000 * 60 * 30;

HMODULE g_hModule           = NULL;
HANDLE g_hIpcEvent          = NULL;
HANDLE g_hSharedMemory      = NULL;
SharedData *g_pSharedData   = NULL;

DWORD g_gameDllSize = 0xF00000;
DWORD g_gameDllBaseAddress = 0;
DWORD g_gameDllEndAddress = 0;
DWORD g_detectedWar3VersionFlag = 0;
std::string g_detectedWar3Version = "";

#ifdef ENABLED_LOG
std::wstring g_dllDirectory;
std::wstring g_wlogWar3HookPath;
#endif

bool g_allHooksInstalled = false;
std::mutex g_installMutex;
std::atomic<HookState> g_hookStatus(HookState::IDLE);
std::atomic<bool> g_isInitialized(false);

std::unordered_map<std::string, TranslatedMessage> g_translatedHistoryMap;
std::mutex g_translatedHistoryMutex;
char g_translateBuffer[1024];

DWORD g_offsetForGameNetEventChatFromHost[2] = {0, 0};
void *g_gameNetEventChatFromHostReturnValue = nullptr;
void *g_gameNetEventChatFromHostReturnAddress = nullptr;
BYTE g_currentBytesForCallGameNetEventChatFromHost[5] = { 0 };
BYTE g_originalBytesForCallGameNetEventChatFromHost[5] = { 0 };
Trampoline g_trampolinesForCallGameNetEventChatFromHost[1] = {{0}};
GameNetEventChatFromHostFunc g_originalGameNetEventChatFromHost = nullptr;
std::map<std::string, DWORD> g_offsetsForGameNetEventChatFromHost[2] = {
    {
        {"1.0.17.5988", 0x6F6DFC},  {"1.0.20.6048", 0x6F72FC},  {"1.20.4.6074", 0x6F7C3C},
        {"1.21.0.6263", 0x6FBC6C},  {"1.22.0.6328", 0x66AEE9},  {"1.23.0.6352", 0x66D2E9},
        {"1.24.1.6374", 0x67BCD9},  {"1.24.4.6387", 0x67BD89},  {"1.25.1.6397", 0x67B3B9},
        {"1.26.0.6401", 0x67B5E9},  {"1.27.0.52240",0x8626E4}
    },
    {
        {"1.0.17.5988", 0x6DEA10},  {"1.0.20.6048", 0x6DEF10},  {"1.20.4.6074", 0x6DF850},
        {"1.21.0.6263", 0x6E37E0},  {"1.22.0.6328", 0x64ABD0},  {"1.23.0.6352", 0x64CB80},
        {"1.24.1.6374", 0x65B570},  {"1.24.4.6387", 0x65B620},  {"1.25.1.6397", 0x65AC50},
        {"1.26.0.6401", 0x65AE80},  {"1.27.0.52240",0x850830}
    }
};

std::vector<std::string> g_signForGameNetEventChatFromHost = { "8B 4C 24 49 2B EE 03 6C 24 34 56 55 51 8B 48 58",
                                                              "E8 ?? ?? ?? ??" };

void initializeSigns()
{
    if (isHighWar3Version()) {

        g_signForGameNetEventChatFromHost = { "8B 4A 58 2B F7 03 75 B4 57 56 FF 75 E9",
                                             "E8 ?? ?? ?? ??" };
    }

    if (isLowerWar3Version()) {

        g_signForGameNetEventChatFromHost = { "8B 4D 0C 2B CB 53 03 4D 10 51 8B 4D F5 51 8B 48 58",
                                             "E8 ?? ?? ?? ??" };
    }
}

// ============================================================================
// DLL 入口点
// ============================================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(NULL, 0, startupHookSystem, NULL, 0, NULL));
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved == NULL) {
            shutdownHookSystem(true);
        }
        break;
    }

    return TRUE;
}

TRANSLATOR_API bool __stdcall initialize()
{
    if (g_isInitialized.load(std::memory_order_acquire)) {
        return true;
    }
    WriteAsyncLogTo(g_wlogWar3HookPath, L"准备初始化 War3Hook");
    initializeSharedMemory();
    std::thread(ipcMessageThread).detach();

    wchar_t processPath[MAX_PATH];
    GetModuleFileNameW(NULL, processPath, MAX_PATH);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"进程路径: %s", processPath);

    g_detectedWar3Version = getCurrentWar3Version();
    if (g_detectedWar3Version.empty()) return false;
    initializeSigns();

    g_gameDllBaseAddress = getModuleBaseAddress("game.dll");
    if (g_gameDllBaseAddress == 0) return false;
    g_gameDllSize = getModuleSize(g_gameDllBaseAddress);
    if (g_gameDllSize == 0) return false;
    g_gameDllEndAddress = g_gameDllBaseAddress + g_gameDllSize;g_offsetForGameNetEventChatFromHost[0] = findVersionOffset(g_offsetsForGameNetEventChatFromHost[0], g_detectedWar3Version, g_signForGameNetEventChatFromHost);
    g_offsetForGameNetEventChatFromHost[1] = findVersionOffset(g_offsetsForGameNetEventChatFromHost[1], g_detectedWar3Version, g_signForGameNetEventChatFromHost, true);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"War3Hook 初始化完成");
    g_isInitialized.store(true, std::memory_order_release);
    return true;
}

bool initializeSharedMemory()
{
    // 1. 创建或打开文件映射对象
    g_hSharedMemory = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        SHARED_MEM_NAME);

    if (!g_hSharedMemory) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 创建共享内存失败，错误码: %d", GetLastError());
        return false;
    }

    // 2. 创建或打开同步事件
    g_hIpcEvent = CreateEventW(NULL, FALSE, FALSE, IPC_EVENT__NAME);

    if (!g_hIpcEvent) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 创建进程事件失败，错误码: %d", GetLastError());
        return false;
    }

    // 检查是否是第一个创建者
    bool isCreator = (GetLastError() != ERROR_ALREADY_EXISTS);

    // 3. 映射物理内存到当前进程空间
    g_pSharedData = (SharedData*)MapViewOfFile(
        g_hSharedMemory,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData));

    if (!g_pSharedData) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 映射共享内存失败，错误码: %d", GetLastError());
        CloseHandle(g_hSharedMemory);
        return false;
    }

    // 4. 仅在首次创建时执行初始化
    if (isCreator)
    {
        // 物理清零整个结构体
        memset(g_pSharedData, 0, sizeof(SharedData));

        // D2L 通道 (Game -> Launcher)
        g_pSharedData->fast_read_index_d2l  = 0;
        g_pSharedData->fast_write_index_d2l = 0;
        g_pSharedData->slow_read_index_d2l  = 0;
        g_pSharedData->slow_write_index_d2l = 0;

        // L2D 通道 (Launcher -> Game)
        g_pSharedData->fast_read_index_l2d  = 0;
        g_pSharedData->fast_write_index_l2d = 0;
        g_pSharedData->slow_read_index_l2d  = 0;
        g_pSharedData->slow_write_index_l2d = 0;

        WriteAsyncLogTo(g_wlogWar3HookPath, L"✨ 共享内存首次创建: 双通道缓冲区索引已归零。");
    }
    else
    {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"🔗 共享内存已由其他进程初始化，当前进程已附加映射。");
    }

    return true;
}

DWORD WINAPI startupHookSystem(LPVOID lpParam)
{
    if(!isWar3Process()) return 0;

    // 1. 等待关键模块加载
    while (GetModuleHandleA("game.dll") == NULL || GetModuleHandleA("storm.dll") == NULL
           || GetModuleHandleA("d3d8.dll") == NULL || GetModuleHandleA("kernel32.dll") == NULL) {
        Sleep(20);
    }
#ifdef ENABLED_LOG
    // 2. 准备日志路径
    g_dllDirectory = getDllDirectoryW(g_hModule);
    g_wlogWar3HookPath = g_dllDirectory + L"logs\\War3Hook.log";
    if (initialize()) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"✅ 初始化成功！执行安装。");
        installAllHooks();
        g_allHooksInstalled = true;
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 初始化失败！取消安装。");
    }
#endif
    return 1;
}

bool shutdownHookSystem(bool isProcessExiting, void* excludeAddress)
{
    // 0. 适用于 FreeLibary 或 手动卸载
#ifdef GET_TEXTURE_LIST
    g_blpStopThread = true;
#endif

    // 1. 先卸载钩子，防止新的API调用进来
    WriteAsyncLogTo(g_wlogWar3HookPath, L"开始手动卸载清理...");

    bool uninstAllHooksSuccess = uninstallAllHooks();

    if (uninstAllHooksSuccess) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"✅ 钩子没有全部卸载，0x%p 稍后卸载！", excludeAddress);
    }
    else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 钩子没有全部卸载！");

    }

    // 2. 清理共享内存
    cleanupSharedMemory();
    WriteAsyncLogTo(g_wlogWar3HookPath, L"共享内存已清理。");

    // 3. 释放为跳板分配的内存
    TrampolineAllocator::uninitialize(isProcessExiting);

    return uninstAllHooksSuccess;
}

void cleanupSharedMemory(bool fullCleanup)
{
    if (fullCleanup) {
        if (g_pSharedData) {
            UnmapViewOfFile(g_pSharedData);
            g_pSharedData = NULL;
        }
        if (g_hSharedMemory) {
            CloseHandle(g_hSharedMemory);
            g_hSharedMemory = NULL;
        }
        WriteAsyncLogTo(g_wlogWar3HookPath, L"🧹 [IPC Buffer] 共享内存已完全卸载并关闭句柄");
    }
    else {
        if (g_pSharedData) {
            memset(g_pSharedData, 0, sizeof(SharedData));
            WriteAsyncLogTo(g_wlogWar3HookPath, L"🧹 [IPC Buffer] 共享内存内容已清空 (保持连接)");
        }
    }
}

TRANSLATOR_API HookState getHookStatus()
{
    return g_hookStatus.load();
}

bool installInlineHook(
    const char *dllName1,
    const char *dllName2,
    const char *hookFuncName,
    void *hookFuncPointer,
    DWORD hookReviseAddressRVA,
    DWORD hookReturnAddressRVA,
    DWORD hookOriginalCallAddressRVA,
    BYTE *originalByteCode,
    BYTE *currentByteCode,
    size_t reviseByteSize,
    HookType hookType,
    Trampoline *trampolines,
    size_t trampolineCount,
    InlineHookResult &result)
{
    result.success = false;

    if (reviseByteSize < 5) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"修改字节小于5");
        return false;
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"开始安装钩子！钩子函数: %S", hookFuncName);

    // 获取模块基地址
    DWORD dllBaseAddress1 = dllName1 ? getModuleBaseAddress(dllName1) : 0;
    DWORD dllBaseAddress2 = dllName2 ? (dllName1 == dllName2 ? dllBaseAddress1 : getModuleBaseAddress(dllName2)) : 0;

    if (inlineHookFilter(dllBaseAddress2 + hookReviseAddressRVA, currentByteCode, reviseByteSize)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"跳过安装: %S 已处于挂钩状态", hookFuncName);
        result.success = true;
        return true;
    }

    result.dllBaseAddress1 = dllBaseAddress1;
    result.dllBaseAddress2 = dllBaseAddress2;

    if (dllBaseAddress2 == 0) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"获取基地2址失败！ 模块名: %S", dllName2 ? dllName2 : "null");
        return false;
    }

    // 计算各个地址
    if (dllBaseAddress2 != 0 && hookReviseAddressRVA != 0) {
        result.reviseAddress = dllBaseAddress2 + hookReviseAddressRVA;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"修改地址: 0x%p", result.reviseAddress);
    }

    if (dllBaseAddress1 != 0 && hookOriginalCallAddressRVA != 0) {
        result.originalCallAddress = dllBaseAddress1 + hookOriginalCallAddressRVA;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"调用地址: 0x%p", result.originalCallAddress);
    }

    if (dllBaseAddress2 != 0 && hookReturnAddressRVA != 0) {
        result.returnAddress = dllBaseAddress2 + hookReturnAddressRVA;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"返回地址: 0x%p", result.returnAddress);
    }

    result.hookFuncAddress = (FARPROC)hookFuncPointer;

    WriteAsyncLogTo(g_wlogWar3HookPath, L"找到钩子函数地址: 0x%p", hookFuncPointer);

    // 执行 inline hook
    if (!hookFunctionByInline(result.reviseAddress,
                              reinterpret_cast<DWORD>(hookFuncPointer),
                              originalByteCode,
                              currentByteCode,
                              reviseByteSize)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"钩子安装失败！");
        return false;
    }

    // 计算 trampoline 总大小
    DWORD maxEndOffset = reviseByteSize + 5;
    for (size_t i = 0; i < trampolineCount; ++i) {
        const Trampoline &tramp = trampolines[i];
        DWORD trampEnd = tramp.offset + tramp.codeSize;
        if (trampEnd > maxEndOffset) {
            maxEndOffset = trampEnd;
        }
    }

    size_t totalSize = maxEndOffset;

    BYTE *mainTrampoline = (BYTE*)TrampolineAllocator::allocate(totalSize);
    if (!mainTrampoline) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 跳板内存池空间不足！");
        uninstallInlineHook(result.reviseAddress, originalByteCode, reviseByteSize);
        return false;
    }

    // 复制原始代码到 trampoline
    memcpy(mainTrampoline, originalByteCode, reviseByteSize);

    // 修复相对指令
    fixRelativeInstructions(mainTrampoline, originalByteCode, reviseByteSize, result.reviseAddress);

    // 设置子 trampoline
    for (size_t i = 0; i < trampolineCount; ++i) {
        Trampoline &tramp = trampolines[i];
        if (tramp.offset + tramp.codeSize > totalSize) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"跳板偏移超出范围: %d", tramp.offset);
            continue;
        }

        tramp.allocatedAddress = mainTrampoline + tramp.offset;
        if (tramp.code && tramp.codeSize > 0) {
            memcpy(tramp.allocatedAddress, tramp.code, tramp.codeSize);
        } else {
            memset(tramp.allocatedAddress, 0x90, tramp.codeSize);
        }

        WriteAsyncLogTo(g_wlogWar3HookPath, L"跳板 [%d] 地址: 0x%p", i, tramp.allocatedAddress);
    }

    // 添加跳转回原函数的指令
    DWORD trampolineEnd = reinterpret_cast<DWORD>(mainTrampoline) + reviseByteSize;
    DWORD jmpBackAddress;
    if (hookType == HookType::Jump) {
        jmpBackAddress = result.returnAddress;
    } else {
        jmpBackAddress = result.reviseAddress + reviseByteSize;
    }

    BYTE jmpBackByteCode[5] = { 0xE9 };
    *reinterpret_cast<DWORD*>(jmpBackByteCode + 1) = jmpBackAddress - (trampolineEnd + 5);
    memcpy(mainTrampoline + reviseByteSize, jmpBackByteCode, 5);

    result.trampolineAddress = mainTrampoline;
    result.success = true;

    WriteAsyncLogTo(g_wlogWar3HookPath, L"钩子安装成功！ 钩子函数: %S", hookFuncName);
    return true;
}

bool hookFunctionByInline(
    DWORD targetAddress,
    DWORD hookAddress,
    BYTE *originalBytes,
    BYTE *currentBytes,
    size_t reviseByteSize)
{
    // 使用足够大的缓冲区，避免溢出
    BYTE intendedJump[16] = { 0 };
    assert(reviseByteSize <= sizeof(intendedJump));
    if (reviseByteSize > sizeof(intendedJump)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"错误: reviseByteSize (%d) 超出缓冲区大小 (%d)", reviseByteSize, sizeof(intendedJump));
        return false;
    }

    DWORD jumpOffset = hookAddress - targetAddress - 5;

    // 构造跳转指令
    intendedJump[0] = 0xE9;
    // 使用 memcpy 是避免严格别名问题的标准做法
    memcpy(&intendedJump[1], &jumpOffset, sizeof(DWORD));

    // 填充剩余字节为 NOP
    for (size_t i = 5; i < reviseByteSize; ++i) {
        intendedJump[i] = 0x90;
    }

    // 调用辅助函数打印将要写入的字节码
#ifdef ENABLED_LOG
    logBytecode(intendedJump, reviseByteSize, L"准备写入的字节码");
#endif
    // 读取原始字节
    BYTE readBytes[16] = { 0 }; // 同样使用足够大的缓冲区
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(targetAddress), &mbi, sizeof(mbi))) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"无法查询目标地址的内存信息, 错误: %d", GetLastError());
        return false;
    }

    if (!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"目标地址不可读，保护属性: 0x%08X", mbi.Protect);
        return false;
    }
    if (targetAddress != 0 && reviseByteSize > 0) {
        memcpy(readBytes, reinterpret_cast<LPCVOID>(targetAddress), reviseByteSize);
    }

    // 检查是否已经 hook
    if (memcmp(readBytes, intendedJump, reviseByteSize) == 0) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"已经安装钩子... 跳过安装");
        return true;
    }

    // 修改内存保护
    DWORD oldProtect;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), reviseByteSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"修改内存保护失败, 错误: %d", GetLastError());
        return false;
    }

    // 保存原始指令
    memcpy(originalBytes, readBytes, reviseByteSize);
#ifdef ENABLED_LOG
    logBytecode(originalBytes, reviseByteSize, L"保存的原始字节码");
#endif
    // 使用逐字节写入，绕开 MinGW 的 memcpy 优化 Bug
    if (targetAddress == 0) {
        return false;
    }
    BYTE *pTarget = reinterpret_cast<BYTE*>(targetAddress);
    for (size_t i = 0; i < reviseByteSize; ++i) {
        pTarget[i] = intendedJump[i];
    }
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(targetAddress), reviseByteSize);

    // 记录当前写入的字节码
    memcpy(currentBytes, intendedJump, reviseByteSize);

    // 恢复内存保护
    VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), reviseByteSize, oldProtect, &oldProtect);

    WriteAsyncLogTo(g_wlogWar3HookPath, L"钩子安装成功！ 地址: 0x%p", targetAddress);
    return true;
}

bool inlineHookFilter(DWORD reviseAddress, BYTE *currentByteCode, size_t reviseByteSize)
{
    if (reviseAddress == 0 || currentByteCode == nullptr || reviseByteSize == 0) {
        return false;
    }

    if (reviseByteSize > 16) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"inlineHookFilter: 请求长度过大 (%zu)", reviseByteSize);
        return false;
    }

    BYTE currentBytes[16] = { 0 };

    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(reinterpret_cast<LPCVOID>(reviseAddress), &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)))
        {
            memcpy(currentBytes, reinterpret_cast<LPCVOID>(reviseAddress), reviseByteSize);
        } else {
            return false;
        }
    } else {
        return false;
    }

    return memcmp(currentBytes, currentByteCode, reviseByteSize) == 0;
}

void fixRelativeInstructions(BYTE *trampoline, const BYTE *original, size_t originalSize, DWORD reviseAddress)
{
    if (trampoline == nullptr || original == nullptr || originalSize == 0) {
        return;
    }

    for (size_t i = 0; i < originalSize; ++i) {
        BYTE opcode = original[i];

        // 1. 处理 E8 / E9 (指令长 5 字节，偏移在 +1 处)
        if (opcode == OPCODE_CALL || opcode == OPCODE_JMP) {
            if (i + 4 < originalSize) {
                DWORD origOffset = 0;
                memcpy(&origOffset, &original[i + 1], sizeof(DWORD));

                DWORD origTarget = reviseAddress + i + 5 + origOffset;
                DWORD newOffset = origTarget - (reinterpret_cast<DWORD>(trampoline) + i + 5);

                memcpy(&trampoline[i + 1], &newOffset, sizeof(DWORD));

                WriteAsyncLogTo(g_wlogWar3HookPath,
                                L"修复相对偏移(5字节指令) %d: 0x%08X -> 0x%08X",
                                i, origOffset, newOffset);

                i += 4;
            }
        }
        // 2. 处理 0F 8x (指令长 6 字节，偏移在 +2 处)
        else if (opcode == OPCODE_TWO_BYTE_PREFIX && (i + 1 < originalSize)) {
            BYTE secondOpcode = original[i + 1];
            if (secondOpcode >= OPCODE_JCC_LONG_MIN && secondOpcode <= OPCODE_JCC_LONG_MAX) {
                if (i + 5 < originalSize) {
                    DWORD origOffset = 0;
                    memcpy(&origOffset, &original[i + 2], sizeof(DWORD));

                    DWORD origTarget = reviseAddress + i + 6 + origOffset;
                    DWORD newOffset = origTarget - (reinterpret_cast<DWORD>(trampoline) + i + 6);

                    memcpy(&trampoline[i + 2], &newOffset, sizeof(DWORD));

                    WriteAsyncLogTo(g_wlogWar3HookPath,
                                    L"修复相对偏移(6字节指令) %d: 0x%08X -> 0x%08X",
                                    i, origOffset, newOffset);

                    i += 5;
                }
            }
        }
    }
}

bool uninstallInlineHook(DWORD targetAddress, BYTE *originalBytes, size_t reviseByteSize)
{
    if (targetAddress == 0 || originalBytes == nullptr) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"目标地址或原字节码不可用");
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(targetAddress), &mbi, sizeof(mbi))) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"无法查询目标地址的内存信息");
        return false;
    }

    {
        BYTE currentBytes[32] = { 0 };
        memcpy(currentBytes, reinterpret_cast<LPCVOID>(targetAddress), reviseByteSize);

        std::wstring byteStr;
        wchar_t buf[8];
        for (size_t i = 0; i < reviseByteSize; ++i) {
            swprintf_s(buf, 8, L"%02X ", currentBytes[i]);
            byteStr += buf;
        }
        WriteAsyncLogTo(g_wlogWar3HookPath, L"恢复前的字节码: %s", byteStr.c_str());
    }

    DWORD oldProtect;
    if (!VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), reviseByteSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"修改内存保护失败, 错误: %d", GetLastError());
        return false;
    }

    memcpy(reinterpret_cast<LPVOID>(targetAddress), originalBytes, reviseByteSize);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPCVOID>(targetAddress), reviseByteSize);

    VirtualProtect(reinterpret_cast<LPVOID>(targetAddress), reviseByteSize, oldProtect, &oldProtect);

    {
        BYTE restoredBytes[32] = { 0 };
        memcpy(restoredBytes, reinterpret_cast<LPCVOID>(targetAddress), reviseByteSize);

        std::wstring byteStr;
        wchar_t buf[8];
        for (size_t i = 0; i < reviseByteSize; ++i) {
            swprintf_s(buf, 8, L"%02X ", restoredBytes[i]);
            byteStr += buf;
        }
        WriteAsyncLogTo(g_wlogWar3HookPath, L"恢复后的字节码: %s", byteStr.c_str());
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"钩子卸载成功！ 地址: 0x%p", targetAddress);
    return true;
}

bool executeHookTask(const wchar_t *label, std::function<bool()> taskFunc, bool isLast, const std::wstring &logPath)
{
    bool ok = taskFunc();
    const wchar_t* prefix = isLast ? TREE_LAST : TREE_ITEM;
    WriteAsyncLogTo(logPath, L"  %s %-45s [%s]", prefix, label, ok ? L"OK ✅" : L"FAIL ❌");
    return ok;
}

TRANSLATOR_API bool __stdcall installAllHooks()
{
    HookState expected = HookState::IDLE;
    if (!g_hookStatus.compare_exchange_strong(expected, HookState::INSTALLING)) {
        if (expected == HookState::INSTALLED) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"ℹ️ [拦截] 钩子已经安装过，无需操作。");
            return true;
        }
        WriteAsyncLogTo(g_wlogWar3HookPath, L"ℹ️ [拦截] 正在安装或卸载中，请勿重复操作。");
        return false;
    }

    std::lock_guard<std::mutex> lock(g_installMutex);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"🚀 开始安装全系统钩子...");

    std::vector<std::wstring> failedHooks;
    bool allOk = true;
    auto task = [&](const wchar_t* lbl, std::function<bool()> f, bool last = false) {
        bool res = executeHookTask(lbl, f, last, g_wlogWar3HookPath);
        if (!res) {
            failedHooks.push_back(lbl);
        }
        allOk &= res;
        return res;
    };

    task(L"Logic: NetEventChatFromHost", []() { return hookGameNetEventChatFromHost(); });

    g_hookStatus.store(allOk ? HookState::INSTALLED : HookState::IDLE);

    if (!allOk) {
        std::wstring failedSummary = L"❌ 失败的钩子: ";
        for (size_t i = 0; i < failedHooks.size(); ++i) {
            failedSummary += failedHooks[i] + (i == failedHooks.size() - 1 ? L"" : L", ");
        }
        WriteAsyncLogTo(g_wlogWar3HookPath, L"%ls", failedSummary.c_str());
    }
    WriteAsyncLogTo(g_wlogWar3HookPath, L"✨ 安装任务结束。状态: %s", allOk ? L"全部成功" : L"部分失败");
    return allOk;
}

TRANSLATOR_API bool __stdcall uninstallAllHooks()
{
    HookState expected = HookState::INSTALLED;
    if (!g_hookStatus.compare_exchange_strong(expected, HookState::UNINSTALLING)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"ℹ️ [拦截] 钩子未安装或正在卸载，跳过。");
        return true;
    }

    std::lock_guard<std::mutex> lock(g_installMutex);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"📂 开始执行全系统卸载流程...");

    bool totalSuccess = true;
    auto task = [&](const wchar_t* lbl, std::function<bool()> f, bool last = false) {
        bool res = executeHookTask(lbl, f, last, g_wlogWar3HookPath);
        totalSuccess &= res;
        return res;
    };
    task(L"Logic: NetEventChatFromHost", unhookGameNetEventChatFromHost);

    // 重置状态
    g_hookStatus.store(HookState::IDLE);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"✨ 卸载任务全部结束。状态: %s", totalSuccess ? L"已完全清空" : L"残留异常");

    return totalSuccess;
}

// ============================================================================
// GameNetEventChatFromHost Hook 实现
// ============================================================================

bool hookGameNetEventChatFromHost()
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"开始安装 game.dll GameNetEventChatFromHost Hook...");

    if(g_gameDllBaseAddress == 0) g_gameDllBaseAddress = getModuleBaseAddress("game.dll");
    if(g_offsetForGameNetEventChatFromHost[0] == 0) g_offsetForGameNetEventChatFromHost[0] = findVersionOffset(g_offsetsForGameNetEventChatFromHost[0], g_detectedWar3Version, g_signForGameNetEventChatFromHost);
    if(g_offsetForGameNetEventChatFromHost[1] == 0) g_offsetForGameNetEventChatFromHost[1] = findVersionOffset(g_offsetsForGameNetEventChatFromHost[1], g_detectedWar3Version, g_signForGameNetEventChatFromHost, true);

    if(g_gameDllBaseAddress == 0 || g_offsetForGameNetEventChatFromHost[0] == 0 || g_offsetForGameNetEventChatFromHost[1] == 0 || !detectAndSetWar3Version()) {
        g_originalGameNetEventChatFromHost = nullptr; // 确保在失败时置空
        return false;
    }

    DWORD gameNetEventChatFromHostAddress0 = g_gameDllBaseAddress + g_offsetForGameNetEventChatFromHost[0];
    DWORD gameNetEventChatFromHostAddress1 = g_gameDllBaseAddress + g_offsetForGameNetEventChatFromHost[1];

    if(!isReadable((void*)gameNetEventChatFromHostAddress0)){
        WriteAsyncLogTo(g_wlogWar3HookPath, L"地址 0x%p 不可读取 基地址: 0x%08X 偏移: 0x%08X", gameNetEventChatFromHostAddress0, g_gameDllBaseAddress, g_offsetForGameNetEventChatFromHost[0]);
        g_originalGameNetEventChatFromHost = nullptr;
        return false;
    }

    if(!isReadable((void*)gameNetEventChatFromHostAddress1)){
        WriteAsyncLogTo(g_wlogWar3HookPath, L"地址 0x%p 不可读取 基地址: 0x%08X 偏移: 0x%08X", gameNetEventChatFromHostAddress1, g_gameDllBaseAddress, g_offsetForGameNetEventChatFromHost[1]);
        g_originalGameNetEventChatFromHost = nullptr;
        return false;
    }

    // 调用 installInlineHook 安装钩子
    InlineHookResult inlineHookResult;
    DWORD rva0 = g_offsetForGameNetEventChatFromHost[0];
    DWORD rva1 = g_offsetForGameNetEventChatFromHost[1];
    bool success = installInlineHook(
        "game.dll",
        "game.dll",
        "jumpWhenCallGameNetEventChatFromHost",
        (void*)jumpWhenCallGameNetEventChatFromHost,
        rva0,
        rva0 + 5,
        rva1,
        g_originalBytesForCallGameNetEventChatFromHost,
        g_currentBytesForCallGameNetEventChatFromHost,
        5,
        HookType::Call,
        g_trampolinesForCallGameNetEventChatFromHost,
        sizeof(g_trampolinesForCallGameNetEventChatFromHost) / sizeof(g_trampolinesForCallGameNetEventChatFromHost[0]),
        inlineHookResult
        );


    if (success) {
        if(isReadable(inlineHookResult.trampolineAddress, sizeof(void*))) {
            g_originalGameNetEventChatFromHost = (GameNetEventChatFromHostFunc)(inlineHookResult.trampolineAddress);
        }

        if(isReadable((void*)inlineHookResult.returnAddress, sizeof(void*))) {
            g_gameNetEventChatFromHostReturnAddress = (void*)inlineHookResult.returnAddress;
        }

        WriteAsyncLogTo(g_wlogWar3HookPath, L"[game.dll] GameNetEventChatFromHost 钩子安装成功。");
        WriteAsyncLogTo(g_wlogWar3HookPath, L"  - 修改地址: 0x%08X", gameNetEventChatFromHostAddress0);
        WriteAsyncLogTo(g_wlogWar3HookPath, L"  - 函数地址: 0x%08X", gameNetEventChatFromHostAddress1);
        WriteAsyncLogTo(g_wlogWar3HookPath, L"  - 返回地址: 0x%08X", inlineHookResult.returnAddress);
        WriteAsyncLogTo(g_wlogWar3HookPath, L"  - 跳板地址: 0x%08X", inlineHookResult.trampolineAddress);
    } else {
        // 如果安装失败，将指针置空以防误用
        g_originalGameNetEventChatFromHost = nullptr;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"[game.dll] GameNetEventChatFromHost 钩子安装失败！");
        freeTrampolineAllocation(&inlineHookResult.trampolineAddress, L"GameNetEventChatFromHost");
    }

    return success;
}

void freeTrampolineAllocation(void **ppAddress, const wchar_t *hookName)
{
    if (ppAddress == nullptr || *ppAddress == nullptr) {
        return;
    }

    void *address = *ppAddress;

    if (VirtualFree(address, 0, MEM_RELEASE)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"🧹 [内存清理] 已释放 [%ls] 的废弃跳板: 0x%p", hookName, address);
        *ppAddress = nullptr;
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [内存清理] 释放 [%ls] 跳板失败 (0x%p), 错误码: %lu",
                        hookName, address, GetLastError());
    }
}

bool unhookGameNetEventChatFromHost()
{
    return uninstallInlineHook(DWORD(g_gameNetEventChatFromHostReturnAddress) - 5, g_originalBytesForCallGameNetEventChatFromHost, 5);
}

#if defined(_MSC_VER)
static __declspec(naked) void __stdcall jumpWhenCallGameNetEventChatFromHost()
{
    __asm {
        pushad
        pushfd

        mov ebp, esp

        lea eax, [ebp + 0x2C]
        push eax
        lea eax, [ebp + 0x28]
        push eax
        mov eax, [ebp + 0x24]
        push eax

        call doSomeThingsBeforeCallGameNetEventChatFromHost

        mov dword ptr [g_gameNetEventChatFromHostReturnValue], eax
        test eax, eax
        jz ReturnZero

        popfd
        popad
        jmp dword ptr [g_originalGameNetEventChatFromHost]

    ReturnZero:
        popfd
        popad
        add esp, 0x0C
        jmp dword ptr [g_gameNetEventChatFromHostReturnAddress]
    }
}
#elif defined(__GNUC__)
__attribute__((naked)) void __stdcall jumpWhenCallGameNetEventChatFromHost() {
    __asm__ __volatile__(
        "pusha\n\t"
        "pushf\n\t"

        "movl %%esp, %%ebp\n\t"

        "leal 0x2C(%%ebp), %%eax\n\t"
        "pushl %%eax\n\t"
        "leal 0x28(%%ebp), %%eax\n\t"
        "pushl %%eax\n\t"
        "movl 0x24(%%ebp), %%eax\n\t"
        "pushl %%eax\n\t"

        "call %0\n\t"

        "movl %%eax, %1\n\t"
        "testl %%eax, %%eax\n\t"
        "jz 1f\n\t"

        "popf\n\t"
        "popa\n\t"
        "jmp *%2\n\t"

        "1:\n\t"
        "popf\n\t"
        "popa\n\t"
        "addl $0x0C, %%esp\n\t"
        "jmp *%3\n\t"
        :
        : "m"(doSomeThingsBeforeCallGameNetEventChatFromHost),
          "m"(g_gameNetEventChatFromHostReturnValue),
          "m"(g_originalGameNetEventChatFromHost),
          "m"(g_gameNetEventChatFromHostReturnAddress)
        : "eax", "memory"
        );
}
#endif

int __stdcall doSomeThingsBeforeCallGameNetEventChatFromHost(int fromPid, void **ppPayload, int *pDataSize)
{
    int realPid = fromPid & 0x000000FF;

    bool isPidInvalid = (realPid < 0 || realPid >= MAX_SLOT);

    if (isPidInvalid) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"⚠ [NetEventChatFromHost] 警告: 检测到异常 PID (原始: 0x%X, 提取: %d)，疑似栈偏移错误",
                        fromPid, realPid);
        return 1;
    }

    if (!isReadable(ppPayload, sizeof(void*)) || !isReadable(pDataSize, sizeof(void*))) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [NetEventChatFromHost] 错误: 传入的参数不可读");
        return 1;
    }

    void *pPayload = *ppPayload;
    int dataSize = *pDataSize;

    if (!isReadable(pPayload, dataSize) || dataSize < 4) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [NetEventChatFromHost] 错误: 数据不可读或长度太短");
        return 1;
    }

    unsigned int chatFlag = *(unsigned int*)pPayload;
    unsigned int extraScope = 0;
    if (chatFlag == 0x20) {
        extraScope = *(unsigned int*)((unsigned char*)pPayload + 1);
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"📥 [NetEventChatFromHost] 收到同步包");
    WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ 原始 PID: 0x%X (提取后: %d)", fromPid, realPid);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ 消息标志: 0x%08X", chatFlag);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ 消息范围: 0x%08X", extraScope);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ 总计长度: %d 字节", dataSize);
    logBytecode((const BYTE*)pPayload, (size_t)dataSize, L"   ├─ 原始字节", true);

    int textOffset  = 4;
    const char *rawMessage = (const char*)pPayload + textOffset;

    if (textOffset < dataSize && *rawMessage == '\0') {
        rawMessage++;
        textOffset++;
    }

    if (isCommand(rawMessage) || isNotWord(rawMessage)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ 检测到无需翻译的消息");
        return 1;
    }

    const char *translatedMessage = nullptr;
    std::wstring wText = utf8ToWide(rawMessage);

    if (wText.empty()) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ 文本内容: [解析失败，首字节为Null]");
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ 文本内容: %ls", wText.c_str());

        if (realPid == 2) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 检测到机器人消息 (PID: %d)", realPid);
            return 2;
        }

        if (g_pSharedData) {
            translatedMessage = getDefaultShoutContent(rawMessage);
            if (!translatedMessage) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 从缓存查找翻译");
                translatedMessage = getTranslationFromCache(rawMessage);
                if (!translatedMessage) {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ 缓存未找到，准备请求翻译并等待结果");
                    requestTranslateMessage(rawMessage, chatFlag, realPid, extraScope);
                    return 0;
                } else {
                    std::wstring wTranslatedMessage = utf8ToWide(translatedMessage);
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"✅ 从缓存中找到翻译，自动转为本地语言: %s", wTranslatedMessage.c_str());
                }
            } else {
                std::wstring wTranslatedMessage = utf8ToWide(translatedMessage);
                WriteAsyncLogTo(g_wlogWar3HookPath, L"✅ 识别到默认喊话，自动转为本地语言: %s", wTranslatedMessage.c_str());
            }
        }
    }

    if (translatedMessage) {
        memcpy(g_translateBuffer, pPayload, textOffset);
        strncpy_s(g_translateBuffer + textOffset, 1024 - textOffset, translatedMessage, _TRUNCATE);
        int newLen = textOffset + (int)strlen(translatedMessage) + 1;
        *ppPayload = g_translateBuffer;
        *pDataSize = newLen;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ ✅ 内容已重定向，新长度: %d", newLen);
    }
    return 1;
}

void ipcMessageThread()
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"🚀 IPC 信号驱动线程已启动 (双通道监控模式)");

    if (g_hIpcEvent == NULL) {
        g_hIpcEvent = CreateEventW(NULL, FALSE, FALSE, IPC_EVENT__NAME);
    }

    while (true) {
        // 1. 等待信号
        DWORD waitResult = WaitForSingleObject(g_hIpcEvent, 100);

        if (waitResult == WAIT_OBJECT_0) {
            // 信号触发
            handleIpcMessages();
        }
        else if (waitResult == WAIT_TIMEOUT) {
            // 2. 轮询检查
            if (g_pSharedData) {
                bool hasFast = (g_pSharedData->fast_read_index_l2d != g_pSharedData->fast_write_index_l2d);
                bool hasSlow = (g_pSharedData->slow_read_index_l2d != g_pSharedData->slow_write_index_l2d);

                if (hasFast || hasSlow) {
                    handleIpcMessages();
                }
            }
        }
        else if (waitResult == WAIT_FAILED) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [IPC] WaitForSingleObject 失败! 错误码: %d", GetLastError());
            Sleep(1000);
        }
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"🛑 IPC 监听线程已安全退出");
}

void handleIpcMessages()
{
    if (!g_pSharedData) return;

    while (g_pSharedData->fast_read_index_l2d != g_pSharedData->fast_write_index_l2d)
    {
        LONG currentReadIdx = g_pSharedData->fast_read_index_l2d;
        MessageSlot &slot = g_pSharedData->fast_buffer_l2d[currentReadIdx];

        // 执行分发处理
        dispatchIpcBufferMessage(slot);

        // 更新 Fast 读指针
        LONG next_read = (currentReadIdx + 1) % BUFFER_SIZE;
        InterlockedExchange(&g_pSharedData->fast_read_index_l2d, next_read);
    }

    while (g_pSharedData->slow_read_index_l2d != g_pSharedData->slow_write_index_l2d)
    {
        LONG currentReadIdx = g_pSharedData->slow_read_index_l2d;
        MessageSlot &slot = g_pSharedData->slow_buffer_l2d[currentReadIdx];

        // 执行分发处理
        dispatchIpcBufferMessage(slot);

        // 更新 Slow 读指针
        LONG next_read = (currentReadIdx + 1) % BUFFER_SIZE;
        InterlockedExchange(&g_pSharedData->slow_read_index_l2d, next_read);
    }
}

void dispatchIpcBufferMessage(const MessageSlot &slot)
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"📥 [IPC/Receive] 来源:%ls | 大小:%d", slot.isFast ? L"FAST" : L"SLOW", slot.size);

    switch (slot.type) {
    case MSG_TYPE_TRANSLATE_RESPONSE:
    {
        if (slot.size >= sizeof(TranslatedResultPayload)) {
            const TranslatedResultPayload *payload = reinterpret_cast<const TranslatedResultPayload*>(slot.payload.data);

            std::string translateLanguage = g_pSharedData->translate_language;

            TranslatedMessage translatedMessage;
            translatedMessage.translatedMessage = payload->translatedMessage;
            translatedMessage.expireTime = GetTickCount64() + CACHE_TTL_MS;

            std::string cacheKey = translateLanguage + ":" + payload->originalMessage;
            {
                std::lock_guard<std::mutex> lock(g_translatedHistoryMutex);
                g_translatedHistoryMap[cacheKey] = translatedMessage;
            }

            std::wstring wOriginalMessage = utf8ToWide(payload->originalMessage);
            std::wstring wTranslatedMessage = utf8ToWide(payload->translatedMessage);

            WriteAsyncLogTo(g_wlogWar3HookPath, L"🧠 [IPC/Cache] 收到 Launcher 翻译回应: ");
            WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ 原文: %ls", wOriginalMessage.c_str());
            WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ 译文: %ls", wTranslatedMessage.c_str());

            WriteAsyncLogTo(g_wlogWar3HookPath, L"🧠 [IPC] 映射已更新，请求 Launcher 上报服务器");
            sendIpcBufferMessage(MSG_TYPE_TRANSLATE_MAPPED, payload, sizeof(TranslatedResultPayload), L"翻译映射添加成功");
        }
        else {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"⚠ [IPC/Cache] 翻译结果包大小异常");
        }
        break;
    }

    default:
        WriteAsyncLogTo(g_wlogWar3HookPath, L"⚠ [IPC Buffer] 未知消息类型: %d", slot.type);
        break;
    }
}

bool isReadable(const void *address, size_t size)
{
    // 1. 基础判空
    if (address == nullptr) {
        return false;
    }

    if (size == 0) {
        return true;
    }

    // 2. 分块检测策略
    char checkBuffer[1024];
    size_t bytesLeft = size;
    const char *currentAddr = (const char*)address;

    while (bytesLeft > 0) {
        // 计算本次读取的大小
        size_t readSize = (bytesLeft > sizeof(checkBuffer)) ? sizeof(checkBuffer) : bytesLeft;

        SIZE_T bytesRead = 0;
        // 尝试读取当前块
        if (!ReadProcessMemory(GetCurrentProcess(), currentAddr, checkBuffer, readSize, &bytesRead)) {
            return false;
        }

        // 再次确认实际读取字节数
        if (bytesRead != readSize) {
            return false;
        }

        bytesLeft -= readSize;
        currentAddr += readSize;
    }

    return true;
}

std::wstring utf8ToWide(const std::string &str)
{
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void safeCopyString(char *target, const char *source, size_t size)
{
    if (!target || size == 0) return;
    if (!source) {
        target[0] = '\0';
        return;
    }
    // 拷贝长度，预留一个字节给 \0
    size_t count = size - 1;
    size_t i = 0;
    for (; i < count && source[i] != '\0'; ++i) {
        target[i] = source[i];
    }
    target[i] = '\0';
}

bool isFastMessage(IpcMessageType type)
{
    switch (type) {
    case MSG_TYPE_TRANSLATE_REQUEST:
        return true;

    default:
        return false;
    }
}

bool isCommand(const char *message)
{
    if (!message || message[0] == '\0') return false;

    // 1. 转换为 std::string 并进行 Trim
    std::string str(message);
    str.erase(0, str.find_first_not_of(" \t\r\n"));
    str.erase(str.find_last_not_of(" \t\r\n") + 1);

    if (str.empty()) return false;

    // 2. 检查前缀特征
    char prefix = str[0];
    if (prefix != '/' && prefix != '-') return false;
    if (str.length() < 2) return false;

    // 3. 转换为小写
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    // 4. 提取第一个单词 (处理带参数的指令，如 "-weather rain")
    size_t spacePos = str.find(' ');
    std::string cmd = (spacePos == std::string::npos) ? str : str.substr(0, spacePos);

    // 5. 核心指令集
    static const std::unordered_set<std::string> knownCommands = {
        // --- 从内存中提取的地图测试/单机指令 ---
        "-lvlup", "-spawncreeps", "-refresh", "-kill", "-gold", "-time", "-powerup",
        "-killall", "-killsent", "-killscourge", "-neutrals", "-noherolimit", "-trees",
        "-killwards", "-spawnoff", "-spawnon", "-roshan", "-respawn", "-dummy", "-wtf", "-test",

        // --- 从内存中提取的英雄/选择相关 ---
        "-ar", "-random", "-repick", "-gameinfo", "-swap", "-recreate", "-unstuck",
        "-matchup", "-ma", "-movespeed", "-ms", "-msa", "-rollhero", "-rh", "-swaphero",

        // --- 从内存中提取的系统/功能开关 ---
        "-disablehelp", "-enablehelp", "-creepstats", "-cson", "-csoff", "-hidemsg",
        "-showmsg", "-weather", "-showdeny", "-hidedeny", "-denyinfo", "-di", "-don",
        "-deathon", "-doff", "-deathoff", "-roll", "-rolloff", "-rollon", "-hhn",
        "-hideheronames", "-mute", "-afk", "-kickafk", "-apm", "-clear", "-courier",
        "-ah", "-invokelist", "-il", "-list", "-music", "-water", "-quote", "-cm",
        "-itemswap", "-bonus", "-rickroll", "-noswitch", "-switch", "-ok", "-no",
        "-terrain", "-tips", "-ii", "-iteminfo", "-center", "-centeroff", "-c", "-co",
        "-unlock", "-disableselection", "-ds", "-enableselection", "-es", "-sleep",
        "-calm", "-nosanta", "-fleshstr", "-switchon",

        // --- 常用地图模式 ---
        "-ap", "-rd", "-sd", "-xl", "-apomnpdu", "-em", "-du", "-om", "-np", "-so",
        "-rd83", "-ap83", "-ar83", "-ff", "-aa", "-st", "-gg", "-am",

        // --- 战网/平台斜杠指令 ---
        "/fps", "/ping", "/w", "/r", "/whois", "/ignore", "/squelch", "/away", "/dnd",
        "/me", "/stats", "/users", "/time", "/join", "/j", "/reready", "/ready"
    };

    // A. 如果在已知列表中，直接确认
    if (knownCommands.count(cmd)) return true;

    // B. 模糊匹配: 如果以 '-' 或 '/' 开头且长度适中，且紧跟字母数字
    if (cmd.length() >= 2 && cmd.length() <= 15) {
        if (std::isalnum(static_cast<unsigned char>(cmd[1]))) {
            return true;
        }
    }

    return false;
}

bool isNotWord(const char *message)
{
    // 1. 空字符串或空指针，视为非单词
    if (!message || message[0] == '\0') return true;

    for (int i = 0; message[i] != '\0'; ++i) {
        unsigned char c = static_cast<unsigned char>(message[i]);

        // 2. 检查是否为标准 ASCII 字母或数字 (A-Z, a-z, 0-9)
        if (std::isalnum(static_cast<unsigned char>(c))) {
            return false;
        }

        // 3. 检查是否为多字节字符
        if (c > 127) {
            return false;
        }
    }

    // 4. 如果遍历完整个字符串，没有发现任何字母、数字或高位字符
    // 说明全是类似 !@#$%^&*()_+ 以及空格、换行等
    return true;
}

void requestTranslateMessage(const char *message, uint32_t flag, uint32_t pid, uint32_t extraScope)
{
    // 1. 基础安全检查
    if (!g_pSharedData) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [request] 丢弃: 共享内存未初始化 (g_pSharedData is NULL)");
        return;
    }

    if (!message || strlen(message) == 0) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [request] 丢弃: 消息内容为空");
        return;
    }

    if (pid > MAX_SLOT) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [request] 丢弃: 检测到异常 PID (%u), 可能是栈偏移错误", pid);
        return;
    }

    // 2. 构造数据
    NotifyTranslatePayload payload = { 0 };
    payload.pid = pid;
    payload.flag = flag;
    payload.extraScope = extraScope;
    safeCopyString(payload.message, message, sizeof(payload.message));

    // 3. 直接发送
    if (!sendIpcBufferMessage(MSG_TYPE_TRANSLATE_REQUEST, &payload, sizeof(payload), L"异步翻译请求")) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [request] IPC 入队失败 (缓冲区满或句柄无效)");
    }
}

const char *getTranslationFromCache(const char *sourceText)
{
    if (!g_pSharedData || !sourceText || strlen(sourceText) == 0) return nullptr;

    std::lock_guard<std::mutex> lock(g_translatedHistoryMutex);

    uint64_t now = GetTickCount64();

    // 1. 逆向查找
    for (auto const& [key, entry] : g_translatedHistoryMap) {
        if (entry.translatedMessage == sourceText) {
            const_cast<TranslatedMessage&>(entry).expireTime = now + CACHE_TTL_MS;
            return sourceText;
        }
    }

    // 2. 正向查找
    std::string translateLanguage = g_pSharedData->translate_language;
    std::string cacheKey = translateLanguage + ":" + sourceText;

    auto it = g_translatedHistoryMap.find(cacheKey);
    if (it != g_translatedHistoryMap.end()) {
        if (now <= it->second.expireTime) {
            it->second.expireTime = now + CACHE_TTL_MS;
            return it->second.translatedMessage.c_str();
        }
    }

    return nullptr;
}

const char *getDefaultShoutContent(const char *content)
{
    if (!g_pSharedData || !content || strlen(content) == 0) return nullptr;

    std::wstring wContent = utf8ToWide(content);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 [getDefaultShoutContent] 开始比对内容: \"%ls\"", wContent.c_str());

    for (int i = 0; i < MAX_SHOUT_ITEMS; ++i) {
        const char *sSrc = g_pSharedData->default_shouts[i].src;
        const char *sDef = g_pSharedData->default_shouts[i].def;

        WriteAsyncLogTo(g_wlogWar3HookPath, L"   ├─ [%d] 锚点: src=\"%ls\", def=\"%ls\"",
                        i, utf8ToWide(sSrc).c_str(), utf8ToWide(sDef).c_str());

        if (strcmp(content, sSrc) == 0) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"   │  └─ ✅ 命中 src (锚点)");
            return sDef;
        }

        if (strcmp(content, sDef) == 0) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"   │  └─ ✅ 命中 def (本地翻译)");
            return sDef;
        }

        bool hasLoggedVariantsHeader = false;
        for (int j = 0; j < MAX_TARGET_LANGUAGES; ++j) {
            const char *variants = g_pSharedData->default_shouts[i].variants[j];

            if (variants[0] == '\0') continue;

            if (!hasLoggedVariantsHeader) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"   │  ├─ 🌐 正在检索变体矩阵...");
                hasLoggedVariantsHeader = true;
            }

            if (strcmp(content, variants) == 0) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"   │  │  └─ ✅ 命中变体 [%d]: \"%ls\"", j, utf8ToWide(variants).c_str());
                return sDef;
            }

            WriteAsyncLogTo(g_wlogWar3HookPath, L"   │  │  - [%d]: %ls", j, utf8ToWide(variants).c_str());
        }
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"   └─ ❌ 矩阵匹配结束，未发现匹配项");
    return nullptr;
}

bool sendIpcBufferMessage(IpcMessageType msgType, const void *data, size_t dataSize, const wchar_t *logTag)
{
    // 1. 指针安全性检查
    if (!g_pSharedData) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [IPC Error] 无法入队: g_pSharedData 为空指针！初始化可能失败。");
        return false;
    }

    // 2. 内部逻辑判定
    bool isFast = isFastMessage(msgType);
    const wchar_t* channelName = isFast ? L"FAST" : L"SLOW";

    // 3. 获取索引快照
    volatile LONG *pWriteIdx = isFast ? &g_pSharedData->fast_write_index_d2l : &g_pSharedData->slow_write_index_d2l;
    volatile LONG *pReadIdx  = isFast ? &g_pSharedData->fast_read_index_d2l  : &g_pSharedData->slow_read_index_d2l;
    MessageSlot   *pBuffer   = isFast ? g_pSharedData->fast_buffer_d2l       : g_pSharedData->slow_buffer_d2l;

    LONG current_write = *pWriteIdx;
    LONG current_read  = *pReadIdx;
    LONG next_write = (current_write + 1) % BUFFER_SIZE;

    // 详细记录当前队列深度
    int pendingCount = (current_write - current_read + BUFFER_SIZE) % BUFFER_SIZE;

    // 4. 检查缓冲区空间
    if (next_write == current_read) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [IPC %ls] 缓冲区满(Count:%d)，丢弃消息: %ls",
                        channelName, pendingCount, logTag);
        return false;
    }

    // 5. 填充数据
    MessageSlot &slot = pBuffer[current_write];
    slot.type = msgType;
    slot.size = (DWORD)dataSize;
    slot.isFast = isFast;

    if (dataSize > 0 && data != nullptr) {
        size_t copyLen = (dataSize > MAX_PACKET_SIZE) ? MAX_PACKET_SIZE : dataSize;
        memcpy(slot.payload.data, data, copyLen);
        if (copyLen < MAX_PACKET_SIZE) slot.payload.data[copyLen] = '\0';
    }

    // 6. 原子提交索引
    InterlockedExchange(pWriteIdx, next_write);

    // 7. 触发事件信号检查
    bool eventTriggered = false;
    if (g_hIpcEvent) {
        if (SetEvent(g_hIpcEvent)) {
            eventTriggered = true;
        } else {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [IPC Error] SetEvent 失败, 错误码: %lu", GetLastError());
        }
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [IPC Error] g_hIpcEvent 句柄无效(NULL)");
    }

    // 8. 成功日志
    WriteAsyncLogTo(g_wlogWar3HookPath,
                    L"✅ [IPC %ls] %ls 已入队 | Slot:%d | Pending:%d | Event:%ls",
                    channelName,
                    logTag,
                    current_write,
                    pendingCount + 1,
                    eventTriggered ? L"OK" : L"FAIL");

    return true;
}

DWORD getModuleBaseAddress(const char *moduleName)
{
    if (!moduleName) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 模块名为空");
        return 0;
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 开始查找模块 [%S]", moduleName);

    // 方法1: 直接获取
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (hModule) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 通过GetModuleHandleA成功获取模块 [%S] 基地址: 0x%08X",
                        moduleName, reinterpret_cast<DWORD>(hModule));
        return reinterpret_cast<DWORD>(hModule);
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: GetModuleHandleA无法找到模块 [%S]，错误代码: %d",
                        moduleName, GetLastError());
    }

    // 方法2: 使用ToolHelp API（推荐）
    DWORD baseAddr = getModuleBaseAddressByToolHelp(moduleName);
    if (baseAddr) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 通过ToolHelp成功获取模块 [%S] 基地址: 0x%08X",
                        moduleName, baseAddr);
        return baseAddr;
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: ToolHelp无法找到模块 [%S]", moduleName);
    }

    // 方法3: 使用PSAPI枚举
    baseAddr = getModuleBaseAddressByEnum(moduleName);
    if (baseAddr) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 通过PSAPI枚举成功获取模块 [%S] 基地址: 0x%08X",
                        moduleName, baseAddr);
    } else {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddress: 所有方法都无法找到模块 [%S]", moduleName);
    }
    return baseAddr;
}

DWORD getModuleBaseAddressByToolHelp(const char *moduleName)
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 开始通过ToolHelp查找模块 [%S]", moduleName);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: CreateToolhelp32Snapshot失败，错误代码: %d", error);
        return 0;
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 创建进程模块快照成功");

    MODULEENTRY32 me32;
    me32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &me32)) {
        int moduleCount = 0;
        do {
            moduleCount++;
            WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 检查模块 [#%d %S] 基地址: 0x%08X 大小: %d",
                            moduleCount, me32.szModule, reinterpret_cast<DWORD>(me32.modBaseAddr), me32.modBaseSize);

            if (_stricmp(me32.szModule, moduleName) == 0) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 找到目标模块 [%S] 基地址: 0x%08X",
                                moduleName, reinterpret_cast<DWORD>(me32.modBaseAddr));
                CloseHandle(hSnapshot);
                return reinterpret_cast<DWORD>(me32.modBaseAddr);
            }
        } while (Module32Next(hSnapshot, &me32));

        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 遍历了 %d 个模块但未找到 [%S]",
                        moduleCount, moduleName);
    } else {
        DWORD error = GetLastError();
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: Module32First失败，错误代码: %d", error);
    }

    CloseHandle(hSnapshot);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByToolHelp: 无法通过ToolHelp找到模块 [%S]", moduleName);
    return 0;
}

DWORD getModuleBaseAddressByEnum(const char *moduleName)
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 开始通过PSAPI枚举查找模块 [%S]", moduleName);

    HANDLE hProcess = GetCurrentProcess();
    HMODULE hModules[1024];
    DWORD cbNeeded;

    // 枚举进程中的所有模块
    if (EnumProcessModules(hProcess, hModules, sizeof(hModules), &cbNeeded)) {
        DWORD moduleCount = cbNeeded / sizeof(HMODULE);
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 枚举到 %d 个模块", moduleCount);

        for (DWORD i = 0; i < moduleCount && i < 1024; i++) {
            char szModuleName[MAX_PATH] = { 0 };

            // 获取模块文件名
            if (GetModuleFileNameExA(hProcess, hModules[i], szModuleName, sizeof(szModuleName))) {
                // 提取纯文件名（不含路径）
                char *fileName = strrchr(szModuleName, '\\');
                if (fileName) fileName++;
                else fileName = szModuleName;

                WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 检查模块 [%S] 基地址: 0x%08X",
                                fileName, reinterpret_cast<DWORD>(hModules[i]));

                // 比较模块名（不区分大小写）
                if (_stricmp(fileName, moduleName) == 0) {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 找到目标模块 [%S] 基地址: 0x%08X",
                                    moduleName, reinterpret_cast<DWORD>(hModules[i]));
                    return reinterpret_cast<DWORD>(hModules[i]);
                }
            } else {
                DWORD error = GetLastError();
                WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 获取模块文件名失败，错误代码: %d", error);
            }
        }

        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 遍历了所有模块但未找到 [%S]", moduleName);
    } else {
        DWORD error = GetLastError();
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: EnumProcessModules失败，错误代码: %d", error);
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"getModuleBaseAddressByEnum: 无法通过PSAPI枚举找到模块 [%S]", moduleName);
    return 0;
}

bool detectAndSetWar3Version()
{
    if (g_detectedWar3Version.empty()) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"版本未检测，尝试自动检测...");
        g_detectedWar3Version = getCurrentWar3Version();

        if (g_detectedWar3Version.empty()) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"无法检测 War3 版本");
            return false;
        }
    }
    return true;
}

std::string getCurrentWar3Version()
{
    WriteAsyncLogTo(g_wlogWar3HookPath, L"getCurrentWar3Version 开始");

    HMODULE hWar3 = GetModuleHandleA("war3.exe");
    if (!hWar3) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"无法获取主模块句柄");
        return "";
    }

    char modulePath[MAX_PATH];
    if (GetModuleFileNameA(hWar3, modulePath, MAX_PATH) == 0) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"无法获取模块文件路径");
        return "";
    }

    // 获取进程名
    char processName[MAX_PATH] = { 0 };
    if (GetModuleBaseNameA(GetCurrentProcess(), NULL, processName, MAX_PATH) == 0) {
        // 失败时从路径提取文件名
        _splitpath_s(modulePath, NULL, 0, NULL, 0, processName, MAX_PATH, NULL, 0);
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"进程名: %hs", processName);

    // 使用 isWar3Exe 函数验证
    if (!isWar3Exe(processName)) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"当前进程不是 War3，进程名: %hs", processName);
        return "";
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"进程名验证通过✅");

    // 获取版本信息
    std::string version = getWar3Version(modulePath);

    if (version.empty()) {
        WriteAsyncLogTo(g_wlogWar3HookPath, L"getWar3Version 返回空版本");
    } else {
        unsigned __int64 currentVal = parseVersionToNumber(version);
        unsigned __int64 limit121 = parseVersionToNumber("1.21.0.6263");
        unsigned __int64 limit126 = parseVersionToNumber("1.26.0.6401");

        if (currentVal <= limit121) {
            g_detectedWar3VersionFlag = 0;
        }
        else if (currentVal <= limit126) {
            g_detectedWar3VersionFlag = 1;
        }
        else {
            g_detectedWar3VersionFlag = 2;
        }

        WriteAsyncLogTo(g_wlogWar3HookPath, L"[VersionFlag] 检测到版本: %hs, 版本标志设为: %d",
                        version.c_str(), g_detectedWar3VersionFlag);
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"getCurrentWar3Version 完成");
    return version;
}

bool isWar3Exe(const char *fullPath)
{
    if (fullPath == NULL) return false;

    const char *fileName = strrchr(fullPath, '\\');
    if (fileName) {
        fileName++;
    } else {
        fileName = fullPath;
    }

    const char *war3Variants[] = {
        "war3.exe",
        "warcraft3.exe",
        "warcraft iii.exe",
        "frozen throne.exe",
        NULL
    };

    for (int i = 0; war3Variants[i] != NULL; i++) {
        if (_stricmp(fileName, war3Variants[i]) == 0) {
            return true;
        }
    }

    if (_stricmp(fileName, "war3") == 0) return true;

    return false;
}

bool isWar3Process()
{
    char path[MAX_PATH];

    if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0) {
        return FALSE;
    }

    if (isWar3Exe(path)) {
        return true;
    }

    return false;
}

unsigned __int64 parseVersionToNumber(const std::string &version)
{
    int v[4] = {0, 0, 0, 0};
    if (sscanf_s(version.c_str(), "%d.%d.%d.%d", &v[0], &v[1], &v[2], &v[3]) != 4) return 0;
    return ((unsigned __int64)v[0] << 48) | ((unsigned __int64)v[1] << 32) |
           ((unsigned __int64)v[2] << 16) | (unsigned __int64)v[3];
}

std::string getWar3Version(const char *war3PathA)
{
    if (!war3PathA) return "";

    // 1. 转换为宽字符路径 (解决中文路径乱码问题)
    int len = MultiByteToWideChar(CP_ACP, 0, war3PathA, -1, NULL, 0);
    if (len <= 0) return "";

    std::wstring war3Path(len, 0);
    MultiByteToWideChar(CP_ACP, 0, war3PathA, -1, &war3Path[0], len);

    // 去除结尾可能的空字符
    if (!war3Path.empty() && war3Path.back() == 0) war3Path.pop_back();

    // 2. 尝试获取 War3.exe 版本
    std::string ver = getFileVersionInternal(war3Path);

    // 3. 如果 War3.exe 无法获取 (例如 1.20e 报错 1813)，尝试 Game.dll
    if (ver.empty()) {
        std::wstring dllDirectory = war3Path;

        // 找到最后一个反斜杠的位置
        size_t lastSlash = dllDirectory.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            // 截断文件名，只保留目录
            dllDirectory = dllDirectory.substr(0, lastSlash + 1);
        } else {
            // 如果没找到斜杠，说明只是文件名，清空目录部分
            dllDirectory = L"";
        }

        // 拼接 Game.dll
        dllDirectory += L"Game.dll";

        // 尝试读取 Game.dll
        ver = getFileVersionInternal(dllDirectory);
    }

    // 4. 如果 Game.dll 还是不行，尝试 Storm.dll (保底)
    if (ver.empty()) {
        std::wstring dllDirectory = war3Path;
        size_t lastSlash = dllDirectory.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            dllDirectory = dllDirectory.substr(0, lastSlash + 1);
        } else {
            dllDirectory = L"";
        }
        dllDirectory += L"Storm.dll";

        ver = getFileVersionInternal(dllDirectory);
    }

    return ver;
}

std::wstring getDllDirectoryW(HMODULE hModule)
{
    wchar_t filePath[MAX_PATH];
    DWORD len = GetModuleFileNameW(hModule, filePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return L".\\";

    std::wstring fullPath(filePath);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return fullPath.substr(0, lastSlash + 1);
    }
    return L".\\";
}

DWORD getModuleSize(DWORD baseAddress)
{
    if (baseAddress == 0) return 0;

    try {
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)baseAddress;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

        PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)(baseAddress + pDosHeader->e_lfanew);
        if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

        return pNtHeaders->OptionalHeader.SizeOfImage;
    }
    catch (...) {
        return 0;
    }
}

std::string getFileVersionInternal(const std::wstring &filePath)
{
    DWORD dummy;
    // 使用 W 版本 API 支持中文路径
    DWORD size = GetFileVersionInfoSizeW(filePath.c_str(), &dummy);
    if (size == 0) {
        return ""; // 获取失败
    }

    std::vector<BYTE> versionInfo(size);
    if (!GetFileVersionInfoW(filePath.c_str(), 0, size, versionInfo.data())) {
        return "";
    }

    VS_FIXEDFILEINFO* fileInfo;
    UINT len;
    if (!VerQueryValueW(versionInfo.data(), L"\\", (LPVOID*)&fileInfo, &len)) {
        return "";
    }

    char version[64];
    snprintf(version, sizeof(version), "%d.%d.%d.%d",
             HIWORD(fileInfo->dwFileVersionMS),
             LOWORD(fileInfo->dwFileVersionMS),
             HIWORD(fileInfo->dwFileVersionLS),
             LOWORD(fileInfo->dwFileVersionLS));

    return std::string(version);
}

bool isHighWar3Version(std::string version, bool includeV126)
{
    // 1. 确定要检测的目标版本号
    std::string targetVersion = version.empty() ? g_detectedWar3Version : version;
    bool isCommon = isCommonWar3Version(targetVersion);
    bool finalResult = false;
    std::wstring logMethod = L"";

    // 2. 判定逻辑
    if (isCommon) {
        logMethod = L"常用版本字符串判断";

        // 核心高版本判定 (1.27+)
        if (targetVersion == "1.27.0.52240")
        {
            finalResult = true;
        }
        // 可选高版本判定 (1.26)
        else if (includeV126 && targetVersion == "1.26.0.6401")
        {
            finalResult = true;
        }
        else {
            finalResult = false;
        }
    }
    else {
        logMethod = L"未知版本数字比对";
        unsigned __int64 currentVal = parseVersionToNumber(targetVersion);
        unsigned __int64 v127Limit = parseVersionToNumber("1.27.0.0");
        unsigned __int64 v126Limit = parseVersionToNumber("1.26.0.0");

        // 如果版本号 >= 1.27.0.0
        if (currentVal >= v127Limit) {
            finalResult = true;
        }
        // 如果包含 1.26 选项，且版本号在 [1.26.0.0, 1.27.0.0) 之间
        else if (includeV126 && currentVal >= v126Limit) {
            finalResult = true;
        }
        else {
            finalResult = false;
        }
    }

    // 3. 记录日志
    WriteAsyncLogTo(g_wlogWar3HookPath,
                    L"[VersionCheck] 正在检测高版本: %hs | 参数 includeV126: %d | 识别方式: %s | 结果: %s",
                    targetVersion.c_str(),
                    includeV126,
                    logMethod.c_str(),
                    finalResult ? L"符合高版本要求" : L"属于低版本");

    return finalResult;
}

bool isLowerWar3Version(std::string version, bool includeV122)
{
    // 1. 确定要检测的目标版本号
    std::string targetVersion = version.empty() ? g_detectedWar3Version : version;
    bool isCommon = isCommonWar3Version(targetVersion);
    bool finalResult = false;
    std::wstring logMethod = L"";

    // 2. 判定逻辑
    if (isCommon) {
        logMethod = L"常用版本字符串判断";
        // 只有 1.21 及以下是原生低版本
        if (targetVersion == "1.0.17.5988" || targetVersion == "1.0.20.6048" ||
            targetVersion == "1.20.4.6074" || targetVersion == "1.21.0.6263")
        {
            finalResult = true;
        }
        // 1.22 根据参数决定是否属于低版本
        else if (includeV122 && targetVersion == "1.22.0.6328")
        {
            finalResult = true;
        }
        else {
            finalResult = false; // 1.23 - 1.27 均为高版本
        }
    }
    else {
        logMethod = L"未知版本数字比对";
        unsigned __int64 currentVal = parseVersionToNumber(targetVersion);
        unsigned __int64 v121Limit = parseVersionToNumber("1.21.0.6263");
        unsigned __int64 v122Val   = parseVersionToNumber("1.22.0.6328");

        if (currentVal <= v121Limit) {
            finalResult = true;
        } else if (includeV122 && (currentVal == v122Val)) {
            finalResult = true;
        } else {
            finalResult = false;
        }
    }

    // 3. 记录日志
    WriteAsyncLogTo(g_wlogWar3HookPath,
                    L"[VersionCheck] 正在检测版本: %hs | 识别方式: %s | 结果: %s",
                    targetVersion.c_str(),
                    logMethod.c_str(),
                    finalResult ? L"低版本x" : L"高版本");

    return finalResult;
}

bool isCommonWar3Version(const std::string &version)
{
    static const std::set<std::string> commonVersions = {
        "1.0.17.5988", "1.0.20.6048", "1.20.4.6074",
        "1.21.0.6263", "1.22.0.6328", "1.23.0.6352",
        "1.24.1.6374", "1.24.4.6387", "1.25.1.6397",
        "1.26.0.6401", "1.27.0.52240"
    };
    return commonVersions.find(version) != commonVersions.end();
}

MultiLevelRVA findVersionOffset(const std::map<std::string, MultiLevelRVA> &versionOffsets, const std::string &version)
{
    // 1. 尝试精确匹配
    auto it = versionOffsets.find(version);
    if (it != versionOffsets.end()) {
        return it->second;
    }

    // 2. 版本未找到，尝试模糊匹配 (只要 Key 包含 version 字符串)
    for (const auto &pair : versionOffsets) {
        if (pair.first.find(version) != std::string::npos) {
            return pair.second;
        }
    }

    // 3. 未找到，返回全 0 结构体
    return {0, 0};
}

DWORD findVersionOffset(const std::map<std::string, DWORD> &versionOffsets, const std::string &version, const std::string &fallbackSignature, bool resolveOpcode)
{
    DWORD offset = 0;
    bool isFromPreset = false;

    // [Log] 1. 打印基础环境信息
    WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 [FindOffset] 开始搜索偏移. 目标版本: %hs", version.c_str());
    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - DLL基址: 0x%08X", g_gameDllBaseAddress);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - DLL大小: %u (0x%X)", g_gameDllSize, g_gameDllSize);

    // 1. 精确匹配
    auto it = versionOffsets.find(version);
    if (it != versionOffsets.end()) {
        offset = it->second;
        isFromPreset = true;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ✅ 精确匹配成功: 预设偏移 0x%X", offset);
    }
    // 2. 模糊匹配
    else {
        for (const auto &pair : versionOffsets) {
            if (pair.first.find(version) != std::string::npos || version.find(pair.first) != std::string::npos) {
                offset = pair.second;
                isFromPreset = true;
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ⚠ 模糊匹配成功: 匹配到 %hs, 偏移 0x%X", pair.first.c_str(), offset);
                break;
            }
        }
    }

    if(isFromPreset && !resolveOpcode) return offset;

    // 3. 兜底: 特征码搜索
    if (offset == 0) {
        if (!fallbackSignature.empty()) {
            if (g_gameDllSize == 0) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ [致命错误] g_gameDllSize 为 0，特征码搜索将立即终止！");
            } else {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - 🔄 尝试特征码搜索 (长度: %d)...", fallbackSignature.length());
                offset = findOffsetBySigns(fallbackSignature);

                if (offset != 0) {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ✅ 特征码匹配成功: 0x%X", offset);
                } else {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ 特征码匹配失败 (返回 0)");
                }
            }
        } else {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ⚠ 未提供特征码，跳过兜底搜索");
        }
    }

    // 4. 处理 Flag 逻辑: 解析 Call 目标
    if (offset != 0) {
        if (resolveOpcode) {
            DWORD address = g_gameDllBaseAddress + offset;

            // 安全检查: 确保地址在模块范围内
            if (address < g_gameDllBaseAddress || address >= g_gameDllBaseAddress + g_gameDllSize) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ [Resolve] 偏移越界！绝对地址 0x%08X 超出模块范围", address);
                return 0;
            }

            // 尝试读取内存
            if (!isReadable((void*)address, 5)) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ [Resolve] 内存不可读: 0x%08X", address);
                return 0;
            }

            BYTE *callPos = (BYTE*)address;
            BYTE opcode = *callPos;
            if (opcode != OPCODE_MOV_EAX_MEM && opcode != OPCODE_MOV_ECX_IMM) return offset;

            // 兼容 E8 (Call) 和 E9 (Jmp) 或 Push (68)
            if (opcode == OPCODE_CALL || opcode == OPCODE_JMP || opcode == OPCODE_PUSH_LONG) {
                int32_t relativeAddress = *(int32_t*)(callPos + 1);
                DWORD targetOffset = offset + 5 + relativeAddress;

                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ↩ [Call] 解析成功. Op: %02X, 相对值: 0x%X -> 目标Offset: 0x%X", opcode, relativeAddress, targetOffset);
                return targetOffset;
            }
            // 兼容 Mov (A1) 或 Mov (B9)
            else if(opcode == OPCODE_MOV_EAX_MEM || opcode == OPCODE_MOV_ECX_IMM) {
                int32_t absoluteAddress = *(int32_t*)(callPos + 1);
                DWORD targetOffset = absoluteAddress - g_gameDllBaseAddress;
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ↩ [Call] 解析成功. Op: %02X, 绝对值: 0x%X -> 目标Offset: 0x%X", opcode, absoluteAddress, targetOffset);
                return targetOffset;
            }
            else {
                if (isFromPreset) {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ℹ️ [Resolve] Opcode: 0x%02X，函数偏移: 0x%X", opcode, offset);
                    return offset;
                } else {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ⚠ [Resolve] Opcode不匹配! 期望 E8/E9, 实际: 0x%02X (地址: 0x%08X)", opcode, address);
                    return 0;
                }
            }
        }
        return offset;
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"❌ [Version] 最终结果: 无法找到偏移");
    return 0;
}

DWORD findVersionOffset(const std::map<std::string, DWORD> &versionOffsets, const std::string &version, const std::vector<std::string> &fallbackSignatures, bool resolveOpcode, bool reverse)
{
    DWORD offset = 0;
    bool isFromPreset = false;

    // [Log] 1. 打印基础环境信息
    WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 [FindOffset] 开始搜索偏移. 目标版本: %hs", version.c_str());
    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - DLL基址: 0x%08X", g_gameDllBaseAddress);
    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - DLL大小: %u (0x%X)", g_gameDllSize, g_gameDllSize);

    // 1. 精确匹配
    auto it = versionOffsets.find(version);
    if (it != versionOffsets.end()) {
        offset = it->second;
        isFromPreset = true;
        WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ✅ 精确匹配成功: 预设偏移 0x%X", offset);
    }
    // 2. 模糊匹配
    else {
        for (const auto &pair : versionOffsets) {
            if (version.find(pair.first) != std::string::npos || pair.first.find(version) != std::string::npos) {
                offset = pair.second;
                isFromPreset = true;
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ⚠ 模糊匹配成功: 匹配到 %hs, 偏移 0x%X", pair.first.c_str(), offset);
                break;
            }
        }
    }

    if(isFromPreset && !resolveOpcode) return offset;

    // 3. 兜底: 多特征码链式搜索
    if (offset == 0) {
        if (!fallbackSignatures.empty() && g_gameDllSize != 0) {
            WriteAsyncLogTo(g_wlogWar3HookPath, L"      - 🔄 尝试多段特征码搜索...");
            offset = findOffsetBySigns(fallbackSignatures, 128, reverse);
            if (offset != 0) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ✅ 特征码链匹配成功: 0x%X", offset);
            }
        }
    }

    // 4. 处理 Flag 逻辑: 解析 Call 目标地址偏移
    if (offset != 0) {
        if (resolveOpcode) {
            DWORD address = g_gameDllBaseAddress + offset;

            // 安全检查: 确保地址在模块范围内
            if (address < g_gameDllBaseAddress || address >= g_gameDllBaseAddress + g_gameDllSize) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ [Resolve] 偏移越界！绝对地址 0x%08X 超出模块范围", address);
                return 0;
            }

            // 尝试读取内存
            if (!isReadable((void*)address, 5)) {
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ❌ [Resolve] 内存不可读: 0x%08X", address);
                return 0;
            }

            BYTE *callPos = (BYTE*)address;
            BYTE opcode = *callPos;
            if (opcode != OPCODE_MOV_EAX_MEM && opcode != OPCODE_MOV_ECX_IMM) return offset;

            // 兼容 Call (E8) 或 Jmp (E9) 或 Push (68)
            if (opcode == OPCODE_CALL || opcode == OPCODE_JMP || opcode == OPCODE_PUSH_LONG) {
                int32_t relativeAddress = *(int32_t*)(callPos + 1);
                DWORD targetOffset = offset + 5 + relativeAddress;
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ↩ [Call] 解析成功. Op: %02X, 相对值: 0x%X -> 目标Offset: 0x%X", opcode, relativeAddress, targetOffset);
                return targetOffset;
            }
            // 兼容 Mov (A1) 或 Mov (B9)
            else if(opcode == OPCODE_MOV_EAX_MEM || opcode == OPCODE_MOV_ECX_IMM) {
                int32_t absoluteAddress = *(int32_t*)(callPos + 1);
                DWORD targetOffset = absoluteAddress - g_gameDllBaseAddress;
                WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ↩ [Call] 解析成功. Op: %02X, 绝对值: 0x%X -> 目标Offset: 0x%X", opcode, absoluteAddress, targetOffset);
                return targetOffset;
            }
            else {
                if (isFromPreset) {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ℹ️ [Resolve] Opcode: 0x%02X，函数偏移: 0x%X", opcode, offset);
                    return offset;
                } else {
                    WriteAsyncLogTo(g_wlogWar3HookPath, L"      - ⚠ [Resolve] Opcode不匹配! 期望: 0x%02X, 实际: 0x%02X (地址: 0x%08X)", resolveOpcode, opcode, address);
                    return 0;
                }
            }
        }
        return offset;
    }

    return 0;
}

DWORD findOffsetBySigns(const std::vector<std::string> &patterns, DWORD maxSearchRange, bool reverse)
{
    if (g_gameDllBaseAddress == 0 || g_gameDllSize == 0 || patterns.empty()) return 0;

    std::vector<std::vector<std::pair<BYTE, bool>>> parsedPatterns;
    for (const auto& p : patterns) parsedPatterns.push_back(parsePattern(p));

    BYTE *mainMenu = (BYTE*)g_gameDllBaseAddress;
    BYTE *pLimit = mainMenu + g_gameDllSize;
    const auto& firstPattern = parsedPatterns[0];

    BYTE *pStart = reverse ? (pLimit - firstPattern.size()) : mainMenu;
    BYTE *pEnd = reverse ? mainMenu : (pLimit - firstPattern.size());
    int step = reverse ? -1 : 1;

    for (BYTE *pCur = pStart; reverse ? (pCur >= pEnd) : (pCur <= pEnd); pCur += step) {
        if (pCur == nullptr) {
            return 0;
        }
        // 1. 匹配第一个特征码
        bool match = true;
        for (size_t i = 0; i < firstPattern.size(); ++i) {
            if (!firstPattern[i].second && pCur[i] != firstPattern[i].first) {
                match = false; break;
            }
        }
        if (!match) continue;

        // 2. 匹配成功，初始化目标位置
        BYTE *pPrevMatchEnd = pCur + firstPattern.size();
        BYTE *pTargetPos = pCur;
        bool allMatched = true;

        for (size_t i = 1; i < parsedPatterns.size(); ++i) {
            const auto& currentPattern = parsedPatterns[i];
            BYTE *pSubSearchLimit = pPrevMatchEnd + maxSearchRange;
            if (pSubSearchLimit > pLimit) pSubSearchLimit = pLimit;

            BYTE *pFoundNext = nullptr;
            for (BYTE *p = pPrevMatchEnd; p <= (pSubSearchLimit - currentPattern.size()); ++p) {
                bool subMatch = true;
                for (size_t j = 0; j < currentPattern.size(); ++j) {
                    if (!currentPattern[j].second && p[j] != currentPattern[j].first) {
                        subMatch = false; break;
                    }
                }
                if (subMatch) { pFoundNext = p; break; }
            }

            if (pFoundNext) {
                pPrevMatchEnd = pFoundNext + currentPattern.size();

                if (!reverse) {
                    pTargetPos = pFoundNext;
                }
            } else {
                allMatched = false; break;
            }
        }

        if (allMatched) {
            DWORD offset = (DWORD)(pTargetPos - mainMenu);
            WriteAsyncLogTo(g_wlogWar3HookPath, L"✅ [Multi-Pattern] 匹配成功！方向: %s, 模式: %s",
                            reverse ? L"反向" : L"正向",
                            reverse ? L"返回首地址" : L"返回尾地址");
            return offset;
        }
    }
    return 0;
}

DWORD findOffsetBySigns(const std::string &pattern)
{
    if (g_gameDllBaseAddress == 0 || g_gameDllSize == 0 || pattern.empty()) {
        return 0;
    }

    auto patternBytes = parsePattern(pattern);
    BYTE *pStart = (BYTE*)g_gameDllBaseAddress;
    BYTE *pEnd = pStart + g_gameDllSize - patternBytes.size();

    // 遍历内存
    for (BYTE *pCurrent = pStart; pCurrent < pEnd; ++pCurrent) {
        bool match = true;

        // 匹配特征码
        for (size_t i = 0; i < patternBytes.size(); ++i) {
            // 如果不是通配符 且 字节不相等，则匹配失败
            if (!patternBytes[i].second && pCurrent[i] != patternBytes[i].first) {
                match = false;
                break;
            }
        }

        if (match) {
            // 找到了！计算相对偏移
            DWORD offset = (DWORD)(pCurrent - pStart);
            WriteAsyncLogTo(g_wlogWar3HookPath, L"🔍 [Pattern] 特征码匹配成功，偏移: 0x%X", offset);
            return offset;
        }
    }

    WriteAsyncLogTo(g_wlogWar3HookPath, L"⚠ [Pattern] 特征码未找到: %hs", pattern.c_str());
    return 0;
}

std::vector<std::pair<BYTE, bool>> parsePattern(const std::string &pattern)
{
    std::vector<std::pair<BYTE, bool>> result;
    std::stringstream ss(pattern);
    std::string byteStr;

    while (ss >> byteStr) {
        if (byteStr == "?" || byteStr == "??" || byteStr == "xx" || byteStr == "XX") {
            result.push_back({ 0, true });
        } else {
            char *pEnd;
            unsigned long val = strtoul(byteStr.c_str(), &pEnd, 16);

            if (pEnd != byteStr.c_str()) {
                result.push_back({ (BYTE)val, false });
            }
        }
    }
    return result;
}

#ifdef ENABLED_LOG
void logBytecode(const BYTE *data, size_t size, const wchar_t *description, bool uppercase)
{
    if (!data || size == 0) return;

    std::wstringstream wss;
    wss << description << L" (" << size << L" bytes): ";

    // 根据参数设置十六进制输出的大小写
    if (uppercase) {
        wss << std::uppercase;
    }

    // 循环格式化每个字节
    for (size_t i = 0; i < size; ++i) {
        wss << std::hex << std::setw(2) << std::setfill(L'0') << static_cast<int>(data[i]) << L" ";
    }
    WriteAsyncLogTo(g_wlogWar3HookPath, wss.str().c_str());
}
#endif