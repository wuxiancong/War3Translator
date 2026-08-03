#include "DebugHelper.h"
#include <QCoreApplication>
#include <QPluginLoader>
#include <QTextStream>
#include <QQmlEngine>
#include <QMetaType>
#include <QDateTime>
#include <QLibrary>
#include <QMutex>
#include <QDebug>
#include <QFile>
#include <QDir>

// 全局日志文件路径
QString logFilePath;

DebugHelper &DebugHelper::instance()
{
    static DebugHelper instance;
    return instance;
}

DebugHelper::DebugHelper(QObject *parent) : QObject(parent)
{
    qDebug().noquote() << "🛠 [DebugHelper] 正在初始化异步日志子系统...";

    // 1. 实例化 Worker
    m_logWorker = new DebugWorker();
    qDebug().noquote() << "   ├─ 📦 DebugWorker 实例已创建";

    // 2. 搬迁到独立线程
    m_logWorker->moveToThread(&m_logThread);
    m_logThread.setObjectName("DebugLogThread");
    qDebug().noquote() << "   ├─ 🧵 Worker 已搬迁至独立线程 [DebugLogThread]";

    // 3. 连接信号槽
    bool c1 = connect(this, &DebugHelper::requestLogWrite, m_logWorker, &DebugWorker::handleWriteLog, Qt::QueuedConnection);

    qDebug().noquote() << QString("   ├─ 🔗 信号链路组装: TreeLog(%1)").arg(c1 ? "✅" : "❌");

    // 4. 资源清理
    connect(&m_logThread, &QThread::finished, m_logWorker, &QObject::deleteLater);

    // 5. 启动线程
    m_logThread.start();

    if (m_logThread.isRunning()) {
        qDebug().noquote() << "   └─ ✅ 异步日志线程已成功启动";
    } else {
        qDebug().noquote() << "   └─ ❌ 严重警告：日志线程启动失败，日志系统可能无法正常工作！";
    }
}

DebugHelper::~DebugHelper()
{
    m_logThread.quit();
    m_logThread.wait();
}

// 记录错误信息到日志文件
void DebugHelper::recordErrorLog(const QString &message)
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);

    if (logFilePath.isEmpty()) {
        logFilePath = QCoreApplication::applicationDirPath() + "/logs/War3launcher_error.log";
    }

    QFile file(logFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out.setCodec("UTF-8");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        out.setGenerateByteOrderMark(true);
#endif

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << "[" << timestamp << "] " << message << "\n";
        file.close();
    }

    // 临时在控制台输出错误，便于调试
    qDebug() << message;
}

bool DebugHelper::checkEnvironment()
{
    recordErrorLog("=== War3Launcher 环境诊断开始 ===");

    QString appDir = QCoreApplication::applicationDirPath();
    recordErrorLog("应用程序目录: " + appDir);
    recordErrorLog("当前工作目录: " + QDir::currentPath());

    return true;
}

bool DebugHelper::checkRequiredFiles()
{
    recordErrorLog("开始检查必要文件");

    QString appDir = QCoreApplication::applicationDirPath();
    bool allFilesOk = true;

    // 检查主程序
    if (!checkFileExists(appDir + "/War3Launcher.exe", "主程序")) {
        allFilesOk = false;
    }

    // 检查关键 Qt DLL
    QStringList criticalDlls = {
        "Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll",
        "Qt5Quick.dll", "Qt5Qml.dll", "Qt5Network.dll"
    };

    for (const QString &dll : criticalDlls) {
        if (!checkFileExists(appDir + "/" + dll, dll)) {
            allFilesOk = false;
        }
    }

    // 检查 MinGW 运行时
    QStringList mingwDlls = {
        "libgcc_s_dw2-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"
    };

    for (const QString &dll : mingwDlls) {
        if (!checkFileExists(appDir + "/" + dll, dll)) {
            allFilesOk = false;
        }
    }

    if (allFilesOk) {
        recordErrorLog("必要文件检查通过");
    } else {
        recordErrorLog("必要文件检查失败");
    }

    return allFilesOk;
}

bool DebugHelper::checkQtPlugins()
{
    recordErrorLog("开始检查 Qt 插件");

    QString appDir = QCoreApplication::applicationDirPath();
    bool allPluginsOk = true;

    // 检查平台插件
    if (!checkDirectoryExists(appDir + "/platforms", "platforms 目录")) {
        allPluginsOk = false;
    } else {
        if (!checkFileExists(appDir + "/platforms/qwindows.dll", "qwindows.dll")) {
            allPluginsOk = false;
        }
    }

    // 检查其他插件目录
    QStringList pluginDirs = {
        "imageformats", "styles"
    };

    for (const QString &dir : pluginDirs) {
        checkDirectoryExists(appDir + "/" + dir, dir + " 目录");
    }

    // 测试加载平台插件
    QPluginLoader platformLoader(appDir + "/platforms/qwindows.dll");
    if (!platformLoader.load()) {
        recordErrorLog("平台插件加载失败: " + platformLoader.errorString());
        allPluginsOk = false;
    } else {
        recordErrorLog("平台插件加载成功");
        platformLoader.unload();
    }

    if (allPluginsOk) {
        recordErrorLog("Qt 插件检查通过");
    } else {
        recordErrorLog("Qt 插件检查失败");
    }

    return allPluginsOk;
}

bool DebugHelper::checkDllDependencies()
{
    recordErrorLog("开始检查 DLL 依赖");

    QString appDir = QCoreApplication::applicationDirPath();
    bool allDllsOk = true;

    // 检查关键 DLL 是否可以加载
    QStringList testDlls = {
        "Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll"
    };

    for (const QString &dllName : testDlls) {
        QLibrary dll(appDir + "/" + dllName);
        if (!dll.load()) {
            recordErrorLog(dllName + " 加载失败: " + dll.errorString());
            allDllsOk = false;
        } else {
            recordErrorLog(dllName + " 加载成功");
            dll.unload();
        }
    }

    if (allDllsOk) {
        recordErrorLog("DLL 依赖检查通过");
    } else {
        recordErrorLog("DLL 依赖检查失败");
    }

    return allDllsOk;
}

bool DebugHelper::checkQmlEngine()
{
    recordErrorLog("开始检查 QML 引擎");

    try {
        QQmlEngine engine;
        QString appDir = QCoreApplication::applicationDirPath();

        // 添加导入路径
        engine.addImportPath(appDir);

        recordErrorLog("QML 引擎初始化成功");
        return true;
    } catch (const std::exception &e) {
        recordErrorLog("QML 引擎初始化失败: " + QString(e.what()));
        return false;
    } catch (...) {
        recordErrorLog("QML 引擎初始化失败: 未知异常");
        return false;
    }
}

bool DebugHelper::checkQmlModules()
{
    recordErrorLog("开始检查 QML 模块");

    QString appDir = QCoreApplication::applicationDirPath();
    bool allModulesOk = true;

    // 检查关键的 QML 模块目录
    QStringList criticalQmlModules = {
        "QtQuick",
        "QtQuick/Window",
        "QtQuick/Layouts",
        "QtQuick/Controls",
        "Qt/labs/platform",
        "Qt/labs/settings"
    };

    for (const QString &modulePath : criticalQmlModules) {
        QString fullPath = appDir + "/" + modulePath;
        if (!QDir(fullPath).exists()) {
            recordErrorLog("QML 模块目录缺失: " + modulePath);
            allModulesOk = false;

            // 尝试在上级目录查找
            QString parentDir = QFileInfo(fullPath).dir().path();
            if (QDir(parentDir).exists()) {
                recordErrorLog("上级目录存在，包含的文件: " + QStringList(QDir(parentDir).entryList(QDir::Dirs | QDir::NoDotAndDotDot)).join(", "));
            }
        } else {
            // 检查目录是否为空
            QDir dir(fullPath);
            QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            if (entries.isEmpty()) {
                recordErrorLog("QML 模块目录为空: " + modulePath);
                allModulesOk = false;
            } else {
                recordErrorLog("QML 模块存在: " + modulePath + " (包含 " + QString::number(entries.count()) + " 个文件)");

                // 对于缺失的 QtQuick/Window 模块，列出 QtQuick 目录下的所有内容
                if (modulePath == "QtQuick") {
                    recordErrorLog("QtQuick 目录内容: " + entries.join(", "));
                }

                // 检查必要的 QML 文件
                checkRequiredQmlFiles(modulePath, fullPath);
            }
        }
    }

    // 检查关键的 QML 插件 DLL
    QStringList criticalQmlPlugins = {
        "Qt5Quick.dll",
        "Qt5Qml.dll",
        "Qt5QmlModels.dll",
        "Qt5QuickControls2.dll"
    };

    for (const QString &plugin : criticalQmlPlugins) {
        if (!QFile::exists(appDir + "/" + plugin)) {
            recordErrorLog("QML 插件 DLL 缺失: " + plugin);
            allModulesOk = false;
        }
    }

    if (allModulesOk) {
        recordErrorLog("QML 模块检查通过");
    } else {
        recordErrorLog("QML 模块检查失败 - 缺少必要的 QML 模块文件");

        // 额外诊断：列出应用程序目录的所有 QML 相关目录
        recordErrorLog("=== QML 相关目录诊断 ===");
        QDir appDirObj(appDir);
        QStringList allDirs = appDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dir : qAsConst(allDirs)) {
            if (dir.startsWith("Qt") || dir == "Qt") {
                QDir subDir(appDir + "/" + dir);
                QStringList subEntries = subDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
                recordErrorLog("目录 " + dir + " 包含: " + subEntries.join(", "));
            }
        }
    }

    return allModulesOk;
}

void DebugHelper::checkRequiredQmlFiles(const QString &modulePath, const QString &fullPath)
{
    // 针对不同模块检查必要的文件
    if (modulePath == "QtQuick/Window") {
        QStringList requiredFiles = {
            "Window.qml", "window.qml", "qmldir", "plugins.qmltypes"
        };

        for (const QString &file : requiredFiles) {
            if (!QFile::exists(fullPath + "/" + file)) {
                recordErrorLog("QtQuick/Window 模块缺少文件: " + file);
            }
        }
    }

    // 检查 qmldir 文件（所有 QML 模块都应该有）
    if (!QFile::exists(fullPath + "/qmldir")) {
        recordErrorLog("QML 模块缺少 qmldir 文件: " + modulePath);
    }
}

void DebugHelper::saveDiagnosticLog()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString logFile = appDir + "/logs/War3launcher_diagnostic.log";

    // 使用 Windows API 直接写入宽字符
    HANDLE hFile = CreateFileW(
        (LPCWSTR)logFile.utf16(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hFile != INVALID_HANDLE_VALUE) {
        // 准备日志内容
        QString content;
        content += "War3Launcher 诊断日志 - " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n";
        content += "========================================\n";
        content += "应用程序目录: " + appDir + "\n";
        content += "Qt 版本: " + QString(QT_VERSION_STR) + "\n";
        content += "Qt 运行时版本: " + QString(qVersion()) + "\n\n";

        content += "文件检查结果:\n";
        QStringList criticalFiles = {
            "War3Launcher.exe", "Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll",
            "Qt5Quick.dll", "Qt5Qml.dll", "libgcc_s_dw2-1.dll",
            "libstdc++-6.dll", "libwinpthread-1.dll"
        };

        for (const QString &fileName : criticalFiles) {
            if (QFile::exists(appDir + "/" + fileName)) {
                content += "✓ " + fileName + "\n";
            } else {
                content += "✗ " + fileName + " (缺失)\n";
            }
        }

        content += "\n目录检查结果:\n";
        QStringList criticalDirs = {"platforms", "imageformats", "styles"};
        for (const QString &dirName : criticalDirs) {
            if (QDir(appDir + "/" + dirName).exists()) {
                content += "✓ " + dirName + "\n";
            } else {
                content += "✗ " + dirName + " (缺失)\n";
            }
        }

        // 详细的 QML 模块检查结果
        content += "\nQML 模块详细检查结果:\n";
        QStringList criticalQmlModules = {
            "QtQuick",
            "QtQuick/Window",
            "QtQuick/Layouts",
            "QtQuick/Controls",
            "Qt/labs/platform",
            "Qt/labs/settings"
        };

        for (const QString &modulePath : criticalQmlModules) {
            QString fullPath = appDir + "/" + modulePath;
            if (QDir(fullPath).exists()) {
                QDir dir(fullPath);
                QStringList entries = dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
                int fileCount = entries.count();
                content += "✓ " + modulePath + " (包含 " + QString::number(fileCount) + " 个文件)\n";

                // 对于 QtQuick 目录，列出所有子目录
                if (modulePath == "QtQuick") {
                    content += "  QtQuick 子目录: " + entries.join(", ") + "\n";
                }

                // 检查关键文件
                if (modulePath == "QtQuick/Window") {
                    QStringList requiredFiles = {"Window.qml", "window.qml", "qmldir", "plugins.qmltypes"};
                    for (const QString &file : requiredFiles) {
                        if (QFile::exists(fullPath + "/" + file)) {
                            content += "  ✓ " + file + "\n";
                        } else {
                            content += "  ✗ " + file + " (缺失)\n";
                        }
                    }
                }
            } else {
                content += "✗ " + modulePath + " (缺失)\n";

                // 检查上级目录是否存在
                QString parentDir = QFileInfo(fullPath).dir().path();
                if (QDir(parentDir).exists()) {
                    QDir parentDirObj(parentDir);
                    QStringList parentEntries = parentDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    content += "  上级目录存在，包含: " + parentEntries.join(", ") + "\n";
                }
            }
        }

        // 应用程序目录结构概览
        content += "\n应用程序目录结构:\n";
        QDir appDirObj(appDir);
        QStringList allDirs = appDirObj.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &dir : qAsConst(allDirs)) {
            if (dir.startsWith("Qt") || dir == "Qt") {
                QDir subDir(appDir + "/" + dir);
                QStringList subEntries = subDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
                content += "  " + dir + "/: " + QString::number(subEntries.count()) + " 个条目\n";
            }
        }

        // 转换为宽字符串并写入
        std::wstring wideContent = content.toStdWString();
        DWORD bytesWritten;

        // 写入 UTF-16 LE BOM
        const wchar_t utf16BOM = 0xFEFF;
        WriteFile(hFile, &utf16BOM, sizeof(utf16BOM), &bytesWritten, NULL);

        // 写入内容
        WriteFile(hFile,
                  wideContent.c_str(),
                  wideContent.length() * sizeof(wchar_t),
                  &bytesWritten,
                  NULL);

        CloseHandle(hFile);

        // 同时在控制台输出关键信息
        qDebug() << "诊断日志已保存到:" << logFile;
        qDebug() << "关键问题: QtQuick/Window 模块缺失";
    } else {
        qDebug() << "无法创建诊断日志文件:" << logFile;
    }
}

bool DebugHelper::checkFileExists(const QString &filePath, const QString &description)
{
    QFile file(filePath);
    if (!file.exists()) {
        recordErrorLog(description + " 缺失: " + filePath);
        return false;
    }
    return true;
}

bool DebugHelper::checkDirectoryExists(const QString &dirPath, const QString &description)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        recordErrorLog(description + " 缺失: " + dirPath);
        return false;
    }
    return true;
}

void DebugHelper::recordRestartEvent(const QString &stage, const QString &details)
{
    QString appDir = QCoreApplication::applicationDirPath();
    // 确保 logs 目录存在
    QDir().mkpath(appDir + "/logs");
    QString logFile = appDir + "/logs/War3launcher_restart.log";

    // 使用 Windows API 以追加模式打开文件
    HANDLE hFile = CreateFileW(
        (LPCWSTR)logFile.utf16(),
        FILE_APPEND_DATA,                   // 追加模式
        FILE_SHARE_READ | FILE_SHARE_WRITE, // 允许其他进程同时读取
        NULL,
        OPEN_ALWAYS,                        // 如果不存在则创建
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hFile != INVALID_HANDLE_VALUE) {
        // 准备日志条目
        QString entry = QString("[%1] [PID:%2] [%3] %4\n")
                            .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
                            .arg(QCoreApplication::applicationPid())
                            .arg(stage.leftJustified(10, ' '))
                            .arg(details);

        // 如果是重启相关的关键阶段，记录当前的命令行参数
        if (stage == "STARTUP" || stage == "RESTART") {
            entry += "   └─ Args: " + QCoreApplication::arguments().join(" ") + "\n";
        }

        std::wstring wideContent = entry.toStdWString();
        DWORD bytesWritten;

        // 如果文件是空的，写入 UTF-16 LE BOM
        if (GetFileSize(hFile, NULL) == 0) {
            const wchar_t utf16BOM = 0xFEFF;
            WriteFile(hFile, &utf16BOM, sizeof(utf16BOM), &bytesWritten, NULL);
        }

        WriteFile(hFile, wideContent.c_str(), wideContent.length() * sizeof(wchar_t), &bytesWritten, NULL);
        CloseHandle(hFile);
    }
}

void DebugHelper::recordTreeLog(const QString &fileName, const QString &message, int level, bool isLast)
{
    LogTask task;
    task.fileName = fileName;
    task.message = message;
    task.level = level;
    task.isLast = isLast;
    task.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    emit instance().requestLogWrite(task);
}
