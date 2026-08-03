#ifndef WAR3LOG_H
#define WAR3LOG_H
#define ENABLED_SYNC_LOG

#include <string>
#include <windows.h>

/**
 * @brief 同步写入日志到指定文件。
 * @details 此函数是线程安全的，并且可以在 DllMain 中安全调用。
 *          日志系统会在第一次调用此函数时自动初始化。
 * @param wlogPath 日志文件的宽字符路径。
 * @param fmt 格式化字符串。
 * @param ... 可变参数。
 */
void writeLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...);

/**
 * @brief 格式化包含文件、行号和函数信息的详细日志消息。
 * @param file 文件名 (__FILE__)。
 * @param line 行号 (__LINE__)。
 * @param function 函数名 (__FUNCTION__)。
 * @param msg 原始日志消息。
 * @return 格式化后的宽字符串。
 */
std::wstring formatSyncMsg(const char *file, int line, const char *function, const wchar_t *msg);

/**
 * @brief 宏，用于方便地调用 formatSyncMsg。
 */
#define FORMAT_SYNC_MSG(msg) formatSyncMsg(__FILE__, __LINE__, __FUNCTION__, (msg))

/**
 * @brief 同步日志宏，Debug 下调用 writeLogTo，Release 下自动移除代码和字符串以节省空间。
 */
#if !defined(ENABLED_SYNC_LOG)
template<typename... Args>
__attribute__((always_inline))
inline void UnusedLogSink(Args... args) {
    ((void)args, ...);
}
#endif
#if defined(ENABLED_SYNC_LOG)
#define WriteLogTo(...) writeLogTo(__VA_ARGS__)
#else
#if defined(_MSC_VER)
#define WriteLogTo(...) __noop(__VA_ARGS__)
#else
#define WriteLogTo(...) UnusedLogSink(__VA_ARGS__)
#endif
#endif

#endif // WAR3LOG_H
