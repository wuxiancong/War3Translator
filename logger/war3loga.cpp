#include "war3loga.h"
#include <strsafe.h>
#include <map>

static std::atomic<bool> g_logSystemInitialized{false};
static std::mutex g_instanceMutex;
static AsyncLogContext* g_pAsyncLogContext = nullptr;
static std::once_flag g_asyncInitOnceFlag;

static size_t calculateTaskMemory(const LogTask& task) {
    return (task.path.size() + task.rawMsg.size()) * sizeof(wchar_t) + sizeof(LogTask) + 32;
}

static std::string wideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string str(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size, nullptr, nullptr);
    return str;
}

static AsyncLogContext *getAsyncLogContext() {
    if (!g_pAsyncLogContext) {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (!g_pAsyncLogContext) g_pAsyncLogContext = new AsyncLogContext();
    }
    return g_pAsyncLogContext;
}

static void logWriterThread() {
    auto *ctx = getAsyncLogContext();
    std::map<std::wstring, HANDLE> handleCache;

    auto GetHandle = [&](const std::wstring& path) -> HANDLE {
        auto it = handleCache.find(path);
        if (it != handleCache.end()) return it->second;
        HANDLE hFile = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) handleCache[path] = hFile;
        return hFile;
    };

    while (true) {
        std::queue<LogTask> localTasks;
        {
            std::unique_lock<std::mutex> lock(ctx->mutex);
            ctx->cv.wait(lock, [ctx] {
                return !ctx->queue.empty() || !ctx->running.load(std::memory_order_acquire);
            });

            if (ctx->queue.empty() && !ctx->running.load(std::memory_order_acquire)) break;

            // 极速交换队列，减少占锁时间
            std::swap(localTasks, ctx->queue);
            ctx->queueSize.store(0, std::memory_order_relaxed);
            // 内存计数在下面循环中扣除
        }

        while (!localTasks.empty()) {
            LogTask& task = localTasks.front();
            size_t taskSize = calculateTaskMemory(task);

            // 1. 生成时间戳
            SYSTEMTIME st;
            GetLocalTime(&st);
            wchar_t finalWBuffer[2560];
            StringCchPrintfW(finalWBuffer, 2560, L"[%02d:%02d:%02d.%03d] %s\r\n",
                             st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, task.rawMsg.c_str());

            // 2. 转换 UTF8
            std::string utf8Str = wideToUtf8(finalWBuffer);

            // 3. 写入文件
            HANDLE hFile = GetHandle(task.path);
            if (hFile != INVALID_HANDLE_VALUE) {
                DWORD written;
                WriteFile(hFile, utf8Str.c_str(), (DWORD)utf8Str.size(), &written, nullptr);
            }

            ctx->totalBytesUsed.fetch_sub(taskSize, std::memory_order_relaxed);
            localTasks.pop();
        }
    }

    for (auto& pair : handleCache) if (pair.second != INVALID_HANDLE_VALUE) CloseHandle(pair.second);
}

size_t getAsyncLogMemoryUsage() {
    auto *ctx = getAsyncLogContext();
    return ctx ? ctx->totalBytesUsed.load(std::memory_order_relaxed) : 0;
}

size_t clearAsyncLogQueue() {
    auto *ctx = getAsyncLogContext();
    if (!ctx) return 0;
    size_t count = 0;
    {
        std::lock_guard<std::mutex> lock(ctx->mutex);
        count = ctx->queue.size();
        std::queue<LogTask> empty;
        std::swap(ctx->queue, empty);
        ctx->queueSize.store(0);
        ctx->totalBytesUsed.store(0);
    }
    return count;
}

void writeAsyncLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...) {
    if (!g_logSystemInitialized.load(std::memory_order_relaxed)) {
        std::call_once(g_asyncInitOnceFlag, []() {
            auto *ctx = getAsyncLogContext();
            ctx->running = true;
            ctx->worker = std::thread(logWriterThread);
            ctx->initialized = true;
            g_logSystemInitialized.store(true);
        });
    }

    if (!g_logSystemInitialized.load(std::memory_order_acquire) || wlogPath.empty() || !fmt) return;

    auto *ctx = g_pAsyncLogContext;

    if (ctx->queueSize.load(std::memory_order_relaxed) > 10000) return;

    wchar_t msgBuffer[2048];
    va_list args;
    va_start(args, fmt);
    StringCchVPrintfW(msgBuffer, 2048, fmt, args);
    va_end(args);

    LogTask task = { wlogPath, msgBuffer };
    size_t taskSize = calculateTaskMemory(task);

    {
        std::lock_guard<std::mutex> lock(ctx->mutex);
        ctx->queue.push(std::move(task));
        ctx->queueSize.fetch_add(1, std::memory_order_relaxed);
        ctx->totalBytesUsed.fetch_add(taskSize, std::memory_order_relaxed);
    }
    ctx->cv.notify_one();
}

bool shutdownLogaSystem(bool forceImmediate) {
    auto *ctx = g_pAsyncLogContext;
    if (!ctx || !ctx->initialized.load()) return false;

    g_logSystemInitialized.store(false);
    if (forceImmediate) clearAsyncLogQueue();

    ctx->running.store(false, std::memory_order_release);
    ctx->cv.notify_all();

    if (ctx->worker.joinable()) ctx->worker.join();
    ctx->initialized.store(false);
    return true;
}

std::wstring formatAsyncMsg(const char *file, int line, const char *function, const wchar_t *msg) {
    wchar_t buf[2048];
    const char* fileName = strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file;
    StringCchPrintfW(buf, 2048, L"[%S:%d] %S: %s", fileName, line, function, msg ? msg : L"");
    return buf;
}