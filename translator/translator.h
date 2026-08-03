#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#ifdef TRANSLATOR_EXPORTS
#define TRANSLATOR_API __declspec(dllexport)
#else
#define TRANSLATOR_API __declspec(dllimport)
#endif

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
#include <map>
#define ENABLED_LOG
typedef void (__fastcall *GameNetEventChatFromHostFunc)(int fromPid, void *pPayload, int dataSize);

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
DWORD WINAPI startupHookSystem(LPVOID lpParam);
DWORD getModuleSize(DWORD baseAddress);
bool isWar3Exe(const char *fullPath);
std::string getCurrentWar3Version();
bool detectAndSetWar3Version();
bool initializeSharedMemory();
void handleIpcMessages();
void ipcMessageThread();
bool isWar3Process();

bool hookGameNetEventChatFromHost();
bool unhookGameNetEventChatFromHost();
void cleanupSharedMemory(bool fullCleanup = true);
bool shutdownHookSystem(bool isProcessExiting, void* excludeAddress = nullptr);
bool freeTrampolineAllocations(bool isProcessExiting, void *excludeAddress = nullptr);
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

void requestTranslateMessage(const char *message, uint32_t flag, uint32_t pid, uint32_t extraScope);
const char *getTranslationFromCache(const char *sourceText);
const char *getDefaultShoutContent(const char *content);

bool sendIpcBufferMessage(IpcMessageType msgType, const void *data, size_t dataSize, const wchar_t *logTag);

int __stdcall doSomeThingsBeforeCallGameNetEventChatFromHost(int fromPid, void **ppPayload, int *pDataSize);

[[maybe_unused]] static void WINAPI jumpWhenCallGameNetEventChatFromHost();
#ifdef ENABLED_LOG
void logBytecode(const BYTE *data, size_t size, const wchar_t *description, bool uppercase = true);;
#endif
extern "C" {
TRANSLATOR_API HookState getHookStatus();
TRANSLATOR_API bool __stdcall initialize();
TRANSLATOR_API bool __stdcall installAllHooks();
TRANSLATOR_API bool __stdcall uninstallAllHooks();
}
#endif // TRANSLATOR_H