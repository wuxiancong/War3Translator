#ifndef WAR3LOGA_H
#define WAR3LOGA_H

#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <windows.h>

#define ENABLED_ASYNC_LOG

struct LogTask {
    std::wstring path;    // 文件路径
    std::wstring message; // 原始宽字符消息
};

struct AsyncLogContext {
    std::queue<LogTask> queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::thread worker;
    std::atomic<bool> initialized{false};
    std::atomic<size_t> queueSize{0};
};

/**
 * @brief 关闭日志系统
 * @param forceImmediate 如果为 true，则立即清空队列并退出，确保魔兽瞬间关闭。
 */
bool shutdownLogaSystem(bool forceImmediate = false);

/**
 * @brief 紧急清空日志队列，释放内存。
 */
size_t clearAsyncLogQueue();

/**
 * @brief 异步写入日志
 */
void writeAsyncLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...);

/**
 * @brief 辅助：格式化包含文件、行号的详细信息
 */
std::wstring formatAsyncMsg(const char *file, int line, const char *function, const wchar_t *msg);

#define FORMAT_ASYNC_MSG(msg) formatAsyncMsg(__FILE__, __LINE__, __FUNCTION__, (msg))

#ifdef ENABLED_ASYNC_LOG
#define WriteAsyncLogTo(...) writeAsyncLogTo(__VA_ARGS__)
#else
#define WriteAsyncLogTo(...) ((void)0)
#endif

#endif // WAR3LOGA_H