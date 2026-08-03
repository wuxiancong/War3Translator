#ifndef IMAGEMANAGER_H
#define IMAGEMANAGER_H

#include <QDir>
#include <QFont>
#include <QDebug>
#include <QObject>
#include <QPixmap>
#include <QPainter>
#include <QFontInfo>
#include <QCoreApplication>
#include <QQuickImageProvider>

class ImageManager : public QObject, public QQuickImageProvider
{
    Q_OBJECT

public:
    // 禁止拷贝和赋值
    ImageManager(const ImageManager&) = delete;
    ImageManager &operator=(const ImageManager&) = delete;

    // 获取单例实例
    static ImageManager &instance();

    // 实现 QQuickImageProvider 的虚函数
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    // 向引擎注册自己
    void registerProvider(QQmlEngine *engine);

    // 创建应用程序图标
    QPixmap createAppIcon(const QSize &size = QSize(64, 64));

    // 基础图标导出
    bool exportAppIcon(const QString &baseName = "translator-app");

    // 多尺寸图标集导出
    bool exportIconSet(const QString &baseName = "translator-app");

    // 自定义图标导出
    bool exportCustomIcon(const QPixmap &pixmap, const QString &baseName);

    // 获取 images 目录路径
    QString getImageDirPath() const;

    // 检查文件是否存在
    bool iconExists(const QString &baseName, const QString &format = "png") const;

    // 获取图标文件路径
    QString getIconPath(const QString &baseName, const QString &format = "png") const;

private:
    explicit ImageManager(QObject *parent = nullptr);
    ~ImageManager();

    // 确保 images 目录存在
    bool ensureImageDirExists();

    // 保存两种格式
    bool saveBothFormats(const QPixmap &pixmap, const QString &baseName);

    // 保存 PNG 格式
    bool savePng(const QPixmap &pixmap, const QString &filePath);

    // 保存 ICO 格式
    bool saveIco(const QPixmap &pixmap, const QString &filePath);

    // 获取支持 emoji 的字体
    QFont getEmojiFont(int pixelSize);
private:
    QString m_imageDir;
};

#endif // IMAGEMANAGER_H
