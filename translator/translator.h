#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#ifdef TRANSLATOR_EXPORTS
#define TRANSLATOR_API __declspec(dllexport)
#else
#define TRANSLATOR_API __declspec(dllimport)
#endif

// 定义定时器 ID
#define TIMER_CHAT_SENDER               9999

#define OPCODE_JCC_LONG_MAX             0x8F
#define OPCODE_JCC_LONG_MIN             0x80
#define OPCODE_TWO_BYTE_PREFIX          0x0F
#define OPCODE_MOV_EAX_MEM              0xA1
#define OPCODE_MOV_ECX_IMM              0xB9
#define OPCODE_PUSH_LONG                0x68
#define OPCODE_CALL                     0xE8
#define OPCODE_JMP                      0xE9

#include "../shared/SharedMemory.h"
#include <functional>
#include <string>
#include <mutex>
#include <map>
#define ENABLED_LOG

struct D3DDEVICE_CREATION_PARAMETERS {
    UINT adapterOrdinal;
    int deviceType;
    HWND hFocusWindow;
    DWORD behaviorFlags;
};

typedef void (__fastcall *GameChatRecipientInGame_Func)(bool networkFlag, const char *text, int recipient, float duration);
typedef void (__fastcall *GameNetEventChatFromHostFunc)(int fromPid, void *pPayload, int dataSize);
typedef void (__fastcall *GameChatInputLogicInGameFunc)(void *eventObject);
typedef void (__fastcall *GamePrimaryGameUIVtable_Func)();
typedef void (__fastcall *GameChatEditBarUIVtable1Func)();
typedef void (__fastcall *GameChatEditBarUIVtable2Func)();
typedef void (__stdcall *GameBeforeD3DXDoEndSceneFunc)();
typedef HRESULT (STDMETHODCALLTYPE *tGetCreationParameters)(
    void **vTablePtr,
    D3DDEVICE_CREATION_PARAMETERS *pParameters
    );

class TrampolineAllocator {
private:
    struct MemoryPool {
        BYTE *base;
        size_t used;
    };

    static inline std::vector<MemoryPool> g_pools;
    static inline const size_t CHUNK_SIZE = 64  *1024;
    static inline std::mutex g_allocMutex;

    static bool createNewPool() {
        BYTE *pNew = (BYTE*)VirtualAlloc(nullptr, CHUNK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!pNew) return false;

        memset(pNew, 0x90, CHUNK_SIZE);
        g_pools.push_back({ pNew, 0 });
        return true;
    }

public:
    static void *allocate(size_t size) {
        std::lock_guard<std::mutex> lock(g_allocMutex);

        // 8字节对齐
        size_t alignedSize = (size + 7)  &~7;
        if (alignedSize > CHUNK_SIZE) return nullptr;

        // 1. 尝试在现有块中寻找可用空间
        BYTE *allocatedAddr = nullptr;

        if (g_pools.empty() || (g_pools.back().used + alignedSize > CHUNK_SIZE)) {
            if (!createNewPool()) return nullptr;
        }

        // 2. 从最后一个池子分配内存
        MemoryPool &currentPool = g_pools.back();
        allocatedAddr = currentPool.base + currentPool.used;
        currentPool.used += alignedSize;

        return (void*)allocatedAddr;
    }

    static bool uninitialize(bool isProcessExiting = false, void *excludeAddress = nullptr) {
        std::lock_guard<std::mutex> lock(g_allocMutex);

        if (g_pools.empty()) return true;

        bool allSuccess = true;

        if (isProcessExiting) {
            g_pools.clear();
            return true;
        }

        std::vector<MemoryPool> preservedPools;

        for (auto &pool : g_pools) {
            if (!pool.base) continue;

            bool shouldExclude = (excludeAddress != nullptr &&
                                  (BYTE*)excludeAddress >= pool.base &&
                                  (BYTE*)excludeAddress < (pool.base + CHUNK_SIZE));

            if (shouldExclude) {
                preservedPools.push_back(pool);
            } else {
                if (!VirtualFree(pool.base, 0, MEM_RELEASE)) {
                    allSuccess = false;
                    preservedPools.push_back(pool);
                }
            }
        }

        g_pools = std::move(preservedPools);
        return allSuccess;
    }
};

enum class HookState {
    IDLE,
    INSTALLING,
    INSTALLED,
    UNINSTALLING
};

enum class HookType {
    Call,
    Jump,
    Other
};

struct MultiLevelRVA {
    DWORD baseOffset;
    DWORD fieldOffset;
};

struct Trampoline {
    DWORD offset;
    BYTE *code;
    size_t codeSize;
    BYTE *allocatedAddress;
};

struct TranslatedMessage {
    std::string translatedMessage;
    uint64_t expireTime;
};

struct TranslatedChatTask {
    uint64_t msgId;
    std::string message;
    uint32_t recipient;
};

LRESULT CALLBACK newWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
std::vector<std::pair<BYTE, bool>> parsePattern(const std::string &pattern);
bool isLowerWar3Version(std::string version = "", bool includeV122 = false);
bool isHighWar3Version(std::string version = "", bool includeV126 = false);
unsigned __int64 parseVersionToNumber(const std::string &version);
std::string getFileVersionInternal(const std::wstring &filePath);
DWORD getModuleBaseAddressByToolHelp(const char *moduleName);
DWORD getModuleBaseAddressByEnum(const char *moduleName);
void dispatchIpcBufferMessage(const MessageSlot &slot);
bool isCommonWar3Version(const std::string &version);
DWORD getModuleBaseAddress(const char *moduleName);
std::string getWar3Version(const char *war3PathA);
std::wstring getDllDirectoryW(HMODULE hModule);
DWORD __stdcall startupHookSystem(LPVOID lpParam);
DWORD getModuleSize(DWORD baseAddress);
void initD3DGlobals(void **vTablePtr);
bool isWar3Exe(const char *fullPath);
std::string getCurrentWar3Version();
void startupHookWindow(HWND hWnd);
bool detectAndSetWar3Version();
bool verifyWar3HookIdentity();
bool initializeSharedMemory();
void handleIpcMessages();
void ipcMessageThread();
bool isWar3Process();

bool hookGameChatRecipientInGame_();
bool unhookGameChatRecipientInGame_();
bool hookGameNetEventChatFromHost();
bool unhookGameNetEventChatFromHost();
bool hookGamePrimaryGameUIVtable_();
bool unhookGamePrimaryGameUIVtable_();
bool hookGameChatEditBarUIVtable1();
bool unhookGameChatEditBarUIVtable1();
bool hookGameChatEditBarUIVtable2();
bool unhookGameChatEditBarUIVtable2();
bool hookGameBeforeD3DXDoEndScene();
bool unhookGameBeforeD3DXDoEndScene();
void cleanupSharedMemory(bool fullCleanup = true);
bool shutdownHookSystem(bool isProcessExiting, void* excludeAddress = nullptr);
bool inlineHookFilter(DWORD reviseAddress, BYTE *currentByteCode, size_t reviseByteSize);
bool uninstallInlineHook(DWORD targetAddress, BYTE *originalBytes, size_t reviseByteSize);
bool executeHookTask(const wchar_t *label, std::function<bool()> taskFunc, bool isLast, const std::wstring &logPath);
bool hookFunctionByInline(DWORD targetAddress, DWORD hookAddress, BYTE *originalBytes, BYTE *currentBytes, size_t reviseByteSize);
void fixRelativeInstructions(BYTE *trampoline, const BYTE *original, size_t originalSize, DWORD reviseAddress);
void freeTrampolineAllocation(void **ppAddress, const wchar_t *hookName);


DWORD findOffsetBySigns(const std::string &pattern);
DWORD findOffsetBySigns(const std::vector<std::string> &patterns, DWORD maxSearchRange, bool reverse);
MultiLevelRVA findVersionOffset(const std::map<std::string, MultiLevelRVA> &versionOffsets, const std::string &version);
DWORD findVersionOffset(const std::map<std::string, DWORD> &versionOffsets, const std::string& version, const std::string &fallbackSignature = "", bool resolveOpcode = false);
DWORD findVersionOffset(const std::map<std::string, DWORD> &versionOffsets, const std::string &version, const std::vector<std::string> &fallbackSignatures, bool resolveOpcode = false, bool reverse = false);

std::wstring utf8ToWide(const std::string &str);
void safeCopyString(char *target, const char *source, size_t size);

bool isCommand(const char *message);
bool isNotWord(const char *message);
bool isFastMessage(IpcMessageType type);
bool isReadable(const void *address, size_t size = 4);

void requestTranslateMessage(const char *message, uint32_t flag, uint32_t pid, uint32_t extraScope, uint64_t currentId, uint32_t direction = 0);
const char *getTranslationFromCache(const char *sourceText);
const char *getDefaultShoutContent(const char *content);

void chatSendGeneral(const char *message, DWORD recipient);
void chatSendInternal(const char *message, DWORD recipient);
bool sendIpcBufferMessage(IpcMessageType msgType, const void *data, size_t dataSize, const wchar_t *logTag);

int __stdcall doSomeThingsBeforeCallGameChatRecipientInGame_(void *gameUI, bool networkFlag, const char *text, int recipient, float duration);
int __stdcall doSomeThingsBeforeCallGameNetEventChatFromHost(int fromPid, void **ppPayload, int *pDataSize);
int __stdcall doSomeThingsBeforeCallGamePrimaryGameUIVtable_(void *primaryGameUI);
int __stdcall doSomeThingsBeforeCallGameChatEditBarUIVtable1(void *chatManager);
int __stdcall doSomeThingsBeforeCallGameChatEditBarUIVtable2(void *chatEditBar);
int __stdcall doSomeThingsBeforeMoveGameBeforeD3DXDoEndScene(void **vTablePtr);

[[maybe_unused]] static void __stdcall jumpWhenCallGameChatRecipientInGame_();
[[maybe_unused]] static void __stdcall jumpWhenCallGameNetEventChatFromHost();
[[maybe_unused]] static void __stdcall jumpWhenCallGamePrimaryGameUIVtable_();
[[maybe_unused]] static void __stdcall jumpWhenCallGameChatEditBarUIVtable1();
[[maybe_unused]] static void __stdcall jumpWhenCallGameChatEditBarUIVtable2();
[[maybe_unused]] static void __stdcall jumpWhenMoveGameBeforeD3DXDoEndScene();
#ifdef ENABLED_LOG
void logBytecode(const BYTE *data, size_t size, const wchar_t *description, bool uppercase = true);;
#endif
extern "C" {
TRANSLATOR_API bool __stdcall initialize();
TRANSLATOR_API bool __stdcall installAllHooks();
TRANSLATOR_API bool __stdcall uninstallAllHooks();
TRANSLATOR_API HookState __stdcall getHookStatus();
TRANSLATOR_API uint32_t __stdcall isWar3TranslatorInitialized();
TRANSLATOR_API void *__stdcall getOriginalGameChatInputLogicInGame();
}
#endif // TRANSLATOR_H
