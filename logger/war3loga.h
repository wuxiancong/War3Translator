#ifndef WAR3LOGA_H
#define WAR3LOGA_H
#define ENABLED_ASYNC_LOG

#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>
#include <condition_variable>

// #ifndef ENABLED_MESSAGE_BOX_W
// #define ENABLED_MESSAGE_BOX_W
// #endif

/**
 * @brief 异步日志系统的核心上下文结构体。
 * @details 包含日志队列、同步原语和工作线程，用于在后台处理日志写入。
 */
struct AsyncLogContext
{
    std::queue<std::pair<std::wstring, std::wstring>> queue; ///< 存储日志任务（路径, 内容）的队列。
    std::mutex mutex;                                        ///< 保护队列访问的互斥锁。
    std::condition_variable cv;                              ///< 用于唤醒后台工作线程的条件变量。
    std::atomic<bool> running{false};                        ///< 控制工作线程生命周期的原子标志。
    std::thread worker;                                      ///< 执行日志写入的后台工作线程。
    std::atomic<bool> initialized{false};                    ///< 标志上下文是否已初始化，防止重复操作。
};

/**
 * @brief 安全地卸载和清理异步日志系统。
 * @details 向后台线程发送停止信号，等待其处理完所有剩余日志后退出，并释放所有相关资源。
 *          应在程序（如 DLL_PROCESS_DETACH）退出前调用。
 */
bool shutdownLogaSystem();

/**
 * @brief 向异步日志系统队列中添加一条日志。
 * @details 日志系统会在第一次调用此函数时自动初始化并启动后台线程。
 * @param wlogPath 目标日志文件的宽字符串路径。
 * @param fmt      格式化字符串 (printf-style)。
 * @param ...      可变参数列表。
 */
void writeAsyncLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...);

/**
 * 为消息添加时间戳和换行符
 * @param message 输入的原始消息
 * @return 格式化后的字符串: [YYYY-MM-DD HH:MM:SS.mmm] message\r\n
 */
std::wstring formatLogWithTime(const wchar_t* message);

/**
 * @brief 格式化包含文件、行号和函数信息的详细日志消息。
 * @param file     源文件名 (__FILE__)。
 * @param line     代码行号 (__LINE__)。
 * @param function 调用函数名 (__FUNCTION__)。
 * @param msg      原始日志消息。
 * @return         格式化后的宽字符串。
 */
std::wstring formatAsyncMsg(const char *file, int line, const char *function, const wchar_t *msg);

/**
 * @brief 宏，用于方便地调用 formatAsyncMsg，自动填充文件、行号和函数信息。
 */
#define FORMAT_ASYNC_MSG(msg) formatAsyncMsg(__FILE__, __LINE__, __FUNCTION__, (msg))

/**
 * @brief 异步日志宏，Debug 下调用 writeAsyncLogTo，Release 下自动移除代码和字符串以节省空间。
 */
#if !defined(ENABLED_ASYNC_LOG)
template<typename... Args>
inline void UnusedLogSink(Args&&...) {}
#endif
#if defined(ENABLED_ASYNC_LOG)
#define USE_ASYNC_LOG 1
#endif
#if defined(USE_ASYNC_LOG)
#define WriteAsyncLogTo(...) writeAsyncLogTo(__VA_ARGS__)
#else
#if defined(_MSC_VER)
#define WriteAsyncLogTo(...) __noop(__VA_ARGS__)
#else
#define WriteAsyncLogTo(...) UnusedLogSink(__VA_ARGS__)
#endif

#endif
#endif // WAR3LOGA_H
