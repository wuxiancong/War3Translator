#include <QQmlApplicationEngine>
#include <QCoreApplication>
#include <QApplication>
#include <QQmlContext>
#include <QIcon>
#include <QQuickWindow>

#include "SingleApplication.h"
#include "monitors/GameMonitor.h"

#include "helpers/FileHelper.h"
#include "helpers/DebugHelper.h"
#include "helpers/ProcessHelper.h"

#include "managers/IpcManager.h"
#include "managers/HookManager.h"
#include "managers/ImageManager.h"
#include "managers/NetworkManager.h"
#include "managers/SettingsManager.h"
#include "managers/TranslateManager.h"

/**
 * @brief 设置应用程序图标逻辑
 */
void setupAppIcon(QApplication &app)
{
    qDebug() << "├── 初始化应用程序图标...";

    if (ImageManager::instance().exportAppIcon("translator-app")) {
        qDebug() << "│   ✅ 图标导出路径:" << ImageManager::instance().getImageDirPath();
    }

    QString appDir = QCoreApplication::applicationDirPath();
    QString iconPath = appDir + "/images/translator-app.ico";

    if (QFile::exists(iconPath)) {
        app.setWindowIcon(QIcon(iconPath));
        qDebug() << "│   ✅ 加载本地图标:" << iconPath;
    } else {
        app.setWindowIcon(QIcon(ImageManager::instance().createAppIcon()));
        qDebug() << "│   ⚠  本地图标不存在，使用生成图标";
    }
}

void setupNetworkManagerThread()
{
    qDebug() << "├── 配置后台网络线程...";
    QThread *networkThread = new QThread();
    networkThread->setObjectName("NetworkBackendThread");

    auto &netManager = NetworkManager::instance();

    netManager.moveToThread(networkThread);

    QObject::connect(networkThread, &QThread::started, &netManager, &NetworkManager::initNetworkManager);

    // 优雅退出逻辑
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [networkThread, &netManager](){
        qDebug() << "🛑 [系统] 收到退出信号，正在清理网络线程...";

        QMetaObject::invokeMethod(&netManager, [&netManager](){
            netManager.gracefulExit();
        }, Qt::BlockingQueuedConnection);

        networkThread->quit();
        networkThread->wait();
        qDebug() << "✅ [系统] 网络线程已安全终止。";
    });

    networkThread->start();
    qDebug() << "│   ✅ 网络线程已启动 (NetworkManager/Detector 已移动)";
}

void setupGameMonitorThread()
{
    qDebug() << "├── 配置后台监视线程...";

    // 1. 创建线程对象
    QThread* monitorThread = new QThread();
    monitorThread->setObjectName("GameMonitorBackendThread");

    // 2. 获取单例引用
    GameMonitor &gamemonitor = GameMonitor::instance();


    // 3. 将单例移动到新线程
    gamemonitor.moveToThread(monitorThread);

    // 4. 连接启动逻辑
    QObject::connect(monitorThread, &QThread::started, &gamemonitor, &GameMonitor::initGameMonitor);

    // 5. 优雅退出逻辑
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, qApp, [monitorThread, &gamemonitor](){
        qDebug() << "🛑 [系统] 收到退出信号，正在清理监视线程...";
        QMetaObject::invokeMethod(&gamemonitor, "cleanup", Qt::BlockingQueuedConnection);

        // 停止事件循环
        monitorThread->quit();

        // 等待线程退出，超时 2 秒则强制终止
        if(!monitorThread->wait(2000)) {
            qWarning() << "⚠ [系统] 监视线程退出超时，强制终止。";
            monitorThread->terminate();
        }

        qDebug() << "✅ [系统] 监视线程已安全终止。";
    });

    // 6. 正式启动线程
    monitorThread->start();
    qDebug() << "│   ✅ 监视线程已启动 (GameMonitor 已移动)";
}

int main(int argc, char *argv[])
{
    // 1. 设置高 DPI 属性
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    // 2. 初始化 SingleApplication
    SingleApplication app(argc, argv, "War3Translator-Instance-Check");

    if (app.isRunning()) {
        qDebug() << "⚠ 检测到程序已在运行，正在激活现有实例并退出当前进程...";
        return 0; // 直接退出当前进程
    }

    // 3. 配置基础设置
    app.setQuitOnLastWindowClosed(false);
    setupAppIcon(app);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appDirPath", QUrl::fromLocalFile(QCoreApplication::applicationDirPath()).toString());

    // 4. 注册核心单例
    qmlRegisterSingletonType(QUrl("qrc:/managers/ThemeManager.qml"), "War3Translator.ThemeManager", 1, 0, "ThemeManager");

    qmlRegisterSingletonType<IpcManager>("War3Translator.IpcManager", 1, 0, "IpcManager",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &IpcManager::instance();
                                          });

    qmlRegisterSingletonType<HookManager>("War3Translator.HookManager", 1, 0, "HookManager",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &HookManager::instance();
                                          });

    qmlRegisterSingletonType<ImageManager>("War3Translator.ImageManager", 1, 0, "ImageManager",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &ImageManager::instance();
                                          });

    qmlRegisterSingletonType<GameMonitor>("War3Translator.GameMonitor", 1, 0, "GameMonitor",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &GameMonitor::instance();
                                          });

    qmlRegisterSingletonType<SettingsManager>("War3Translator.SettingsManager", 1, 0, "SettingsManager",
                                              [](QQmlEngine*, QJSEngine*) -> QObject *{
                                                  return &SettingsManager::instance();
                                              });

    qmlRegisterSingletonType<TranslateManager>("War3Translator.TranslateManager", 1, 0, "TranslateManager",
                                               [](QQmlEngine*, QJSEngine*) -> QObject *{
                                                   return &TranslateManager::instance();
                                               });

    qmlRegisterSingletonType<NetworkManager>("War3Launcher.NetworkManager", 1, 0, "NetworkManager",
                                             [](QQmlEngine*, QJSEngine*) -> QObject* { return &NetworkManager::instance(); });

    qmlRegisterSingletonType<FileHelper>("War3Translator.FileHelper", 1, 0, "FileHelper",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &FileHelper::instance();
                                          });

    qmlRegisterSingletonType<DebugHelper>("War3Translator.DebugHelper", 1, 0, "DebugHelper",
                                          [](QQmlEngine*, QJSEngine*) -> QObject *{
                                              return &DebugHelper::instance();
                                          });

    qmlRegisterSingletonType<ProcessHelper>("War3Translator.ProcessHelper", 1, 0, "ProcessHelper",
                                               [](QQmlEngine*, QJSEngine*) -> QObject *{
                                                   return &ProcessHelper::instance();
                                               });

    // 5. 加载 QML
    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [url](QObject *obj, const QUrl &objUrl) {
                         if (!obj && url == objUrl)
                             QCoreApplication::exit(-1);
                     }, Qt::QueuedConnection);

    engine.load(url);

    // 6. 处理多实例唤醒逻辑
    QObject::connect(&app, &SingleApplication::instanceStarted, [&engine]() {
        qDebug() << "🌐 [系统] 收到外部唤醒请求，正在恢复窗口...";
        // 获取 QML 中的根窗口对象
        QObject *root = engine.rootObjects().value(0);
        if (root) {
            QQuickWindow *window = qobject_cast<QQuickWindow*>(root);
            if (window) {
                window->show();             // 显示窗口
                window->raise();            // 窗口置顶
                window->requestActivate();  // 请求焦点
            }
        }
    });
    setupGameMonitorThread();
    setupNetworkManagerThread();

    return app.exec();
}