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
    std::wstring path;
    std::wstring rawMsg;
};

struct AsyncLogContext {
    std::queue<LogTask> queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{false};
    std::thread worker;
    std::atomic<bool> initialized{false};
    std::atomic<size_t> queueSize{0};
    std::atomic<size_t> totalBytesUsed{0};
};

/**
 * @brief 关闭日志系统
 * @param forceImmediate 为 true 时瞬间清空队列并退出，不等待写入，用于 War3 退出。
 */
bool shutdownLogaSystem(bool forceImmediate = false);

/**
 * @brief 获取当前日志积压占用的内存字节数
 */
size_t getAsyncLogMemoryUsage();

/**
 * @brief 紧急清空日志队列并返回清空的条数
 */
size_t clearAsyncLogQueue();

/**
 * @brief 异步写入日志
 */
void writeAsyncLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...);

/**
 * @brief 格式化详细消息宏辅助
 */
std::wstring formatAsyncMsg(const char *file, int line, const char *function, const wchar_t *msg);

#define FORMAT_ASYNC_MSG(msg) formatAsyncMsg(__FILE__, __LINE__, __FUNCTION__, (msg))

#ifdef ENABLED_ASYNC_LOG
#define WriteAsyncLogTo(...) writeAsyncLogTo(__VA_ARGS__)
#else
#define WriteAsyncLogTo(...) ((void)0)
#endif

#endif