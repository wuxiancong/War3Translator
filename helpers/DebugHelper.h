#ifndef DEBUGHELPER_H
#define DEBUGHELPER_H

#include <QMutex>
#include <QObject>
#include <QThread>

#include "../workers/DebugWorker.h"

class DebugHelper : public QObject
{
    Q_OBJECT

public:
    // 禁止拷贝和赋值
    DebugHelper(const DebugHelper&) = delete;
    DebugHelper& operator=(const DebugHelper&) = delete;

    // 获取单例实例
    static DebugHelper &instance();

    static bool checkEnvironment();
    static bool checkRequiredFiles();
    static bool checkQtPlugins();
    static bool checkDllDependencies();
    static bool checkQmlEngine();
    static bool checkQmlModules();
    static void saveDiagnosticLog();
    static void recordErrorLog(const QString &message);
    static void recordRestartEvent(const QString &stage, const QString &details);
    Q_INVOKABLE static void recordTreeLog(const QString &fileName, const QString &message, int level, bool isLast = false);

signals:
    void requestLogWrite(const LogTask &task);

private:
    // 构造函数私有化
    explicit DebugHelper(QObject *parent = nullptr);
    ~DebugHelper();

    // 非静态实现方法
    bool checkEnvironmentImpl();
    bool checkRequiredFilesImpl();
    bool checkQtPluginsImpl();
    bool checkDllDependenciesImpl();
    bool checkQmlEngineImpl();
    bool checkQmlModulesImpl();
    void saveDiagnosticLogImpl();

    // 静态辅助方法
    static bool checkFileExists(const QString &filePath, const QString &description);
    static bool checkDirectoryExists(const QString &dirPath, const QString &description);
    static void checkRequiredQmlFiles(const QString &modulePath, const QString &fullPath);


    QThread m_logThread;
    DebugWorker *m_logWorker;
};

#endif // DEBUGHELPER_H
