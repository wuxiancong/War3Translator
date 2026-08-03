#include "war3log.h"
#include <strsafe.h>

// 全局临界区对象，用于保护对日志文件的访问
static CRITICAL_SECTION g_logCriticalSection;
// 初始化标志，防止重复初始化/销毁
volatile LONG g_logInitialized = 0;

/**
 * @brief 一个RAII类，用于自动管理同步日志系统的生命周期。
 * @details 构造函数初始化临界区，析构函数负责销毁。
 *          通过在 `writeLogTo` 中创建静态实例，确保初始化和清理的自动化和线程安全。
 */
struct SyncLogManager {
    SyncLogManager() {
        InitializeCriticalSection(&g_logCriticalSection);
    }
    ~SyncLogManager() {
        DeleteCriticalSection(&g_logCriticalSection);
    }
};

/**
 * @brief [内部] 宽字符串到UTF-8字符串的转换辅助函数。
 * @param wstr 待转换的宽字符串。
 * @return     转换后的UTF-8字符串。
 */
static std::string wideToUtf8Sync(const std::wstring &wstr)
{
    if (wstr.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};

    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);

    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }

    return result;
}

/**
 * @brief [内部] 将UTF-8编码的日志文本写入文件的核心IO函数。
 * @details 包含一个简单的日志文件大小检查和截断逻辑 (限制 200KB)。
 * @param wlogPath 日志文件的宽字符串路径。
 * @param wlogText 要写入的日志内容的宽字符串。
 */
static void writeLogUtf8Sync(const std::wstring &wlogPath, const std::wstring &wlogText)
{
    if (wlogPath.empty() || wlogText.empty()) {
        return;
    }

    // 1. 设置文件大小限制
    const DWORD maxSize = 1024 * 1024 * 2; // 2Mb

    // 2. 检查文件大小，如果超过限制则清空
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    // 使用 GetFileAttributesExW 获取文件大小，避免打开文件的开销
    if (GetFileAttributesExW(wlogPath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        // 如果文件大小 >= 2Mb(只检查低位即可，日志通常不会超过4GB)
        if (fileInfo.nFileSizeLow >= maxSize && fileInfo.nFileSizeHigh == 0) {
            // 使用 TRUNCATE_EXISTING 标志打开文件，这将把文件长度截断为 0
            HANDLE hFile = CreateFileW(wlogPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                       TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            // 只要打开成功，截断即完成，无需写入，直接关闭
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
            }
        }
    }

    // 3. 以追加模式打开文件并写入日志
    HANDLE hFile = CreateFileW(wlogPath.c_str(),
                               FILE_APPEND_DATA,      // 追加数据权限
                               FILE_SHARE_READ,       // 允许其他程序读取(如记事本)
                               nullptr,
                               OPEN_ALWAYS,           // 文件不存在则创建，存在则打开
                               FILE_ATTRIBUTE_NORMAL,
                               nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        // 将宽字符日志转换为 UTF-8 编码，避免乱码
        std::string utf8Log = wideToUtf8Sync(wlogText);

        if (!utf8Log.empty()) {
            DWORD bytesWritten;
            // 执行写入
            WriteFile(hFile, utf8Log.c_str(), static_cast<DWORD>(utf8Log.size()), &bytesWritten, nullptr);
        }

        CloseHandle(hFile);
    }
}

void writeLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...)
{
    // C++11及以上标准保证局部静态变量的初始化是线程安全的。
    // 这行代码确保 SyncLogManager 的构造函数（即 InitializeCriticalSection）
    // 在多线程环境中只被调用一次。
    static SyncLogManager manager;

    if (wlogPath.empty() || !fmt) {
        return;
    }

    // [格式化和写入逻辑保持不变]
    wchar_t buffer[2048];
    va_list args;
    va_start(args, fmt);
    HRESULT hr = StringCchVPrintfW(buffer, _countof(buffer), fmt, args);
    va_end(args);

    if (FAILED(hr)) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[128];
    StringCchPrintfW(timeStr, _countof(timeStr),
                     L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    std::wstring finalLog = std::wstring(timeStr) + buffer + L"\r\n";

    EnterCriticalSection(&g_logCriticalSection);
    writeLogUtf8Sync(wlogPath, finalLog);
    LeaveCriticalSection(&g_logCriticalSection);
}

std::wstring formatSyncMsg(const char *file, int line, const char *function, const wchar_t *msg)
{
    if (!file || !function) {
        return L"无效的文件或函数信息";
    }

    // 将ANSI源码信息转换为宽字符
    wchar_t wfile[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, file, -1, wfile, MAX_PATH - 1);

    wchar_t wfunction[256] = {0};
    MultiByteToWideChar(CP_ACP, 0, function, -1, wfunction, 255);

    // 格式化最终输出
    wchar_t buf[2048] = {0};
    swprintf_s(buf, _countof(buf) - 1, L"文件: %s \r\n行号: %d \r\n函数: %s \r\n信息: %s",
               wfile, line, wfunction, msg ? msg : L"");

    return std::wstring(buf);
}
