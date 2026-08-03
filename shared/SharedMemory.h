// SharedMemory.h

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <cstdint>
#include <windows.h>

// 定义共享内存的名称，必须在两个进程中完全一致
#define SHARED_MEM_NAME L"Global\\War3Translator_SharedMemory"
#define IPC_EVENT__NAME L"Global\\War3Translator_L2D_Event"

#define MAX_SLOT 16
#define BUFFER_SIZE 1024
#define MAX_PACKET_SIZE 1500

#define MAX_SHOUT_ITEMS 7
#define MAX_SHOUT_TEXT_LEN 128
#define MAX_TARGET_LANGUAGES 25

const uint16_t PROTOCOL_MAGIC        = 0xF801;                  // 魔数
const uint8_t  PROTOCOL_VERSION      = 0x01;                    // 版本号

enum PacketType : uint8_t {
    C_S_HEARTBEAT                   = 0x02,
    C_S_REGISTER                    = 0x03,
    S_C_REGISTER                    = 0x04,
    C_S_UNREGISTER                  = 0x05,
    C_S_COMMAND                     = 0x09,
    C_S_PING                        = 0x0D,
    S_C_PONG                        = 0x0E,
    C_S_TRANSLATED_MESSAGE          = 0x25
};

enum IpcMessageType : DWORD {
    MSG_TYPE_TRANSLATE_REQUEST,                                 // DLL -> Launcher: 让 Launcher 翻译
    MSG_TYPE_TRANSLATE_RESPONSE,                                // Launcher -> DLL: 返回翻译内容到 dll
    MSG_TYPE_TRANSLATE_MAPPED                                   // DLL -> Launcher: 让 Launcher 发送
};

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t magic;
    uint8_t  version;
    uint8_t  command;
    uint32_t sessionId;
    uint64_t seq;
    uint16_t payloadLen;
    uint16_t checksum;
    char    signature[16];
};

struct CSRegisterPacket {
    char clientId[40];
    char hardwareId[65];
    char userName[32];
    char localIp[16];
    char publicIp[16];
    uint16_t localPort;
    uint16_t publicPort;
    uint8_t natType;
};

struct SCRegisterPacket {
    uint32_t sessionId;
    uint8_t status;
};

struct SCPongPacket {
    uint8_t status;
};

struct CSCommandPacket {
    char clientId[40];
    char userName[32];
    char command[16];
    char text[200];
};

struct DefaultShoutData {
    char src[MAX_SHOUT_TEXT_LEN];                               // 原始中文，例如 "上路不见了..."
    char def[MAX_SHOUT_TEXT_LEN];                               // 当前 UI 语言对应的翻译
    char variants[MAX_TARGET_LANGUAGES][MAX_SHOUT_TEXT_LEN];
};

struct InlineHookResult {
    bool success = false;
    DWORD dllBaseAddress1 = 0;
    DWORD dllBaseAddress2 = 0;
    DWORD reviseAddress = 0;
    DWORD returnAddress = 0;
    DWORD originalCallAddress = 0;
    FARPROC hookFuncAddress = nullptr;
    void *trampolineAddress = nullptr;
};

struct NotifyTranslatePayload {
    uint32_t            pid;                                        // 发送者 PID
    uint32_t            flag;                                       // 消息标志 (0x10/0x20)
    uint32_t            extraScope;                                 // 消息范围 (0/1/2/3/A)
    char                message[512];                               // 消息内容 (UTF-8)
};

struct TranslatedResultPayload {
    uint32_t            pid;
    uint32_t            flag;
    uint32_t            extraScope;
    char                originalMessage[512];                       // 原文 (作为 Map 的 Key)
    char                translatedMessage[512];                     // 译文 (作为 Map 的 Value)
};

struct CSTranslatedMessagePacket {
    uint32_t            pid;
    uint32_t            flag;
    uint32_t            extraScope;
    char                message[512];
};

struct MessageSlot {
    IpcMessageType      type;
    DWORD               size;                                       // 对于数据包，是包的长度；对于连接请求，是sockaddr_in的大小
    SOCKET              socket;                                     // 关联的socket句柄
    bool                isFast;                                     // 标记是否为快速通道消息

    union {
        sockaddr_in     addr;                                       // 用于 MSG_TYPE_CONNECT_REQUEST
        char            data[MAX_PACKET_SIZE];                      // 用于数据包
    } payload;
};

struct SharedData {
    char                translate_language[8];
    DefaultShoutData    default_shouts[MAX_SHOUT_ITEMS];

    volatile LONG       fast_read_index_d2l;
    volatile LONG       fast_write_index_d2l;
    MessageSlot         fast_buffer_d2l[BUFFER_SIZE];

    volatile LONG       fast_read_index_l2d;
    volatile LONG       fast_write_index_l2d;
    MessageSlot         fast_buffer_l2d[BUFFER_SIZE];

    volatile LONG       slow_read_index_d2l;
    volatile LONG       slow_write_index_d2l;
    MessageSlot         slow_buffer_d2l[BUFFER_SIZE];

    volatile LONG       slow_read_index_l2d;
    volatile LONG       slow_write_index_l2d;
    MessageSlot         slow_buffer_l2d[BUFFER_SIZE];
};
#pragma pack(pop)
#endif // SHARED_MEMORY_H
