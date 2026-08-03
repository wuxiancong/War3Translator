#include "war3loga.h"
#include <strsafe.h>

// 全局变量区

/**
 * @brief 全局临界区，用于在后台线程中同步文件写入，以及在降级模式下直接写入时提供保护。
 */
static CRITICAL_SECTION g_logCriticalSection;

/**
 * @brief 标志整个日志系统是否已初始化。
 */
static std::atomic<bool> g_logSystemInitialized{false};

/**
 * @brief 用于保护 g_asyncLogContext 单例创建的互斥锁。
 */
static std::mutex g_instanceMutex;

/**
 * @brief 使用指针管理的全局异步日志上下文单例。
 * @details 避免了“静态对象析构顺序问题”，确保资源被正确管理。
 */
static AsyncLogContext* g_pAsyncLogContext = nullptr;

/**
 * @brief 用于确保异步日志系统初始化逻辑只执行一次的标志。
 */
static std::once_flag g_asyncInitOnceFlag;

// 内部辅助函数

/**
 * @brief 获取异步日志上下文的单例实例。
 * @details 采用双重检查锁定模式，确保线程安全地创建唯一实例。
 * @return 指向 AsyncLogContext 实例的指针。
 */
static AsyncLogContext* getAsyncLogContext()
{
    if (!g_pAsyncLogContext) {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (!g_pAsyncLogContext) {
            g_pAsyncLogContext = new AsyncLogContext(); // 手动申请，不自动释放
        }
    }
    return g_pAsyncLogContext;
}

/**
 * @brief [内部] 宽字符串到UTF-8字符串的转换辅助函数。
 * @param wstr 待转换的宽字符串。
 * @return     转换后的UTF-8字符串。
 */
static std::string wideToUtf8Async(const std::wstring &wstr)
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
 * @details 包含一个简单的日志文件大小检查和截断逻辑。
 * @param wlogPath 日志文件的宽字符串路径。
 * @param wlogText 要写入的日志内容的宽字符串。
 */
static void writeLogUtf8Async(const std::wstring &wlogPath, const std::wstring &wlogText)
{
    if (wlogPath.empty() || wlogText.empty()) {
        return;
    }

    const DWORD maxSize = 1024 * 1024 * 10; // 10Mb

    // 检查文件大小，如果超过限制则清空
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (GetFileAttributesExW(wlogPath.c_str(), GetFileExInfoStandard, &fileInfo)) {
        if (fileInfo.nFileSizeLow >= maxSize && fileInfo.nFileSizeHigh == 0) {
            HANDLE hFile = CreateFileW(wlogPath.c_str(), GENERIC_WRITE, 0, nullptr,
                                       TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
        }
    }

    // 以追加模式打开文件并写入日志
    HANDLE hFile = CreateFileW(wlogPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        std::string utf8Log = wideToUtf8Async(wlogText);
        if (!utf8Log.empty()) {
            DWORD bytesWritten;
            WriteFile(hFile, utf8Log.c_str(), (DWORD)utf8Log.size(), &bytesWritten, nullptr);
        }
        CloseHandle(hFile);
    }
}


/**
 * @brief 后台工作线程的主函数。
 * @details 循环地从队列中取出日志任务并写入文件，直到收到停止信号。
 */
static void logWriterThread()
{
    auto *ctx = getAsyncLogContext();
    if (!ctx) return;

    while (ctx->running.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock(ctx->mutex);
        // 等待队列不为空或收到停止信号
        ctx->cv.wait(lock, [ctx] {
            return !ctx->queue.empty() || !ctx->running.load(std::memory_order_acquire);
        });

        // 批量处理队列中的日志，减少锁的竞争
        std::queue<std::pair<std::wstring, std::wstring>> tasks;
        tasks.swap(ctx->queue);
        lock.unlock();

        while (!tasks.empty()) {
            auto& task = tasks.front();

            // 增加双重检查：如果主系统已经进入 shutdown，停止写磁盘
            // 因为此时文件系统句柄可能已经失效
            if (!g_logSystemInitialized.load(std::memory_order_acquire)) break;

            EnterCriticalSection(&g_logCriticalSection);
            writeLogUtf8Async(task.first, task.second);
            LeaveCriticalSection(&g_logCriticalSection);
            tasks.pop();
        }
    }

    // 线程即将退出，处理队列中所有剩余的日志
    std::unique_lock<std::mutex> lock(ctx->mutex);
    while (!ctx->queue.empty()) {
        auto task = std::move(ctx->queue.front());
        ctx->queue.pop();
        lock.unlock(); // 尽快释放锁

        EnterCriticalSection(&g_logCriticalSection);
        writeLogUtf8Async(task.first, task.second);
        LeaveCriticalSection(&g_logCriticalSection);

        lock.lock(); // 准备下一次循环
    }
    // --- 调试点 ---
#ifdef ENABLED_MESSAGE_BOX_W
    MessageBoxW(NULL, L"日志后台写线程已接收到停止信号并退出。",
                L"Thread Debug", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
#endif
}

bool shutdownLogaSystem()
{
    auto *ctx = g_pAsyncLogContext;

    // 1. 检查是否需要清理
    if (!ctx || !ctx->initialized.load(std::memory_order_acquire)) {
        return false;
    }

    // --- 调试点 1 ---
#ifdef ENABLED_MESSAGE_BOX_W
    MessageBoxW(NULL, L"[Step 1] 进入日志清理函数，准备发送停止信号...", L"War3Hook Debug", MB_OK | MB_SYSTEMMODAL);
#endif
    // 2. 停止工作线程循环
    ctx->running.store(false, std::memory_order_release);
    ctx->cv.notify_all();

    // --- 调试点 2 ---
#ifdef ENABLED_MESSAGE_BOX_W
    MessageBoxW(NULL, L"[Step 2] 停止信号已发送，准备执行 worker.join()...\n（如果点完这个卡住了，说明卡在了等待日志写完的阶段）", L"War3Hook Debug", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
#endif
    // 3. 等待线程结束
    if (ctx->worker.joinable()) {
        ctx->worker.join();
    }

    // --- 调试点 3 ---
#ifdef ENABLED_MESSAGE_BOX_W
    MessageBoxW(NULL, L"[Step 3] worker.join() 成功返回！线程已安全退出。", L"War3Hook Debug", MB_OK | MB_ICONASTERISK | MB_SYSTEMMODAL);
#endif
    // 4. 清理全局状态标志
    g_logSystemInitialized.store(false, std::memory_order_release);
    ctx->initialized.store(false, std::memory_order_release);

    // --- 调试点 4 ---
#ifdef ENABLED_MESSAGE_BOX_W
    MessageBoxW(NULL, L"[Step 4] 日志系统已彻底关闭，返回魔兽争霸3原生流程。", L"War3Hook Debug", MB_OK | MB_SYSTEMMODAL);
#endif

    return true;
}

void writeAsyncLogTo(const std::wstring &wlogPath, const wchar_t *fmt, ...)
{
    // 使用 std::call_once 确保初始化代码在多线程环境下只被执行一次。
    // 这会在第一次调用 writeAsyncLogTo 时触发。
    std::call_once(g_asyncInitOnceFlag, []() {
        InitializeCriticalSection(&g_logCriticalSection);

        auto *ctx = getAsyncLogContext();
        if (ctx) {
            ctx->running = true;
            ctx->worker = std::thread(logWriterThread);
            ctx->initialized = true;
        }
        g_logSystemInitialized.store(true, std::memory_order_release);
    });

    if (!g_logSystemInitialized.load(std::memory_order_acquire)) {
        return; // 如果初始化失败，则直接返回
    }

    if (wlogPath.empty() || !fmt) return;

    // [格式化和入队逻辑保持不变]
    wchar_t buffer[2048]; // 2kb
    va_list args;
    va_start(args, fmt);
    StringCchVPrintfW(buffer, _countof(buffer), fmt, args);
    va_end(args);

    auto *ctx = getAsyncLogContext();
    if (ctx && ctx->running.load(std::memory_order_acquire)) {
        {
            std::lock_guard<std::mutex> lock(ctx->mutex);
            ctx->queue.emplace(wlogPath, std::move(formatLogWithTime(buffer)));
        }
        ctx->cv.notify_one();
    } else {
        EnterCriticalSection(&g_logCriticalSection);
        writeLogUtf8Async(wlogPath,  formatLogWithTime(buffer));
        LeaveCriticalSection(&g_logCriticalSection);
    }
}

std::wstring formatLogWithTime(const wchar_t* message)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t timeStr[128];
    StringCchPrintfW(timeStr, _countof(timeStr),
                     L"[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                     st.wYear, st.wMonth, st.wDay,
                     st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    return std::wstring(timeStr) + message + L"\r\n";
}

std::wstring formatAsyncMsg(const char *file, int line, const char *function, const wchar_t *msg)
{
    if (!file || !function) return L"Invalid file or function information";

    wchar_t wfile[MAX_PATH] = {0};
    MultiByteToWideChar(CP_ACP, 0, file, -1, wfile, MAX_PATH - 1);

    wchar_t wfunction[256] = {0};
    MultiByteToWideChar(CP_ACP, 0, function, -1, wfunction, 255);

    wchar_t buf[2048] = {0};
    swprintf_s(buf, _countof(buf) - 1, L"文件: %s \r\n行号: %d \r\n函数: %s \r\n信息: %s",
               wfile, line, wfunction, msg ? msg : L"");

    return std::wstring(buf);
}
