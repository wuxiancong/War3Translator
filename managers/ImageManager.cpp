#include "../managers/ImageManager.h"

ImageManager &ImageManager::instance()
{
    static ImageManager instance;
    return instance;
}

ImageManager::ImageManager(QObject *parent)
    : QObject(parent), QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    m_imageDir = QCoreApplication::applicationDirPath() + "/images";
    ensureImageDirExists();

    qDebug() << "ImageManager (Provider) 初始化, 目录:" << m_imageDir;
}

ImageManager::~ImageManager()
{

}

QPixmap ImageManager::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    // 1. 去掉 QML 传来的查询参数
    QString pureId = id.split('?').at(0);

    // 2. 拼接完整磁盘路径
    QString filePath = m_imageDir + "/" + pureId;

    QPixmap pixmap(filePath);
    if (pixmap.isNull()) {
        qDebug() << "❌ [ImageProvider] 图片不存在:" << filePath;
        pixmap = QPixmap(requestedSize.isValid() ? requestedSize : QSize(64, 64));
        pixmap.fill(Qt::transparent);
    }

    if (size) *size = pixmap.size();
    return pixmap;
}

void ImageManager::registerProvider(QQmlEngine *engine)
{
    if (!engine) return;
    engine->addImageProvider(QLatin1String("war3"), this);
    qDebug() << "✅ 已注册 ImageProvider: image://war3/";
}

bool ImageManager::exportAppIcon(const QString &baseName)
{
    QPixmap pixmap = createAppIcon(QSize(64, 64));
    return saveBothFormats(pixmap, baseName);
}

bool ImageManager::exportIconSet(const QString &baseName)
{
    QVector<QSize> sizes = {
        QSize(16, 16),
        QSize(32, 32),
        QSize(48, 48),
        QSize(64, 64),
        QSize(128, 128),
        QSize(256, 256)
    };

    bool allSuccess = true;

    for (const QSize &size : sizes) {
        QPixmap pixmap = createAppIcon(size);
        QString name = QString("%1_%2x%2").arg(baseName).arg(size.width());

        if (!saveBothFormats(pixmap, name)) {
            allSuccess = false;
            qWarning() << "导出文件失败，大小:" << size;
        } else {
            qDebug() << "导出文件成功，大小:" << size;
        }
    }

    // 导出主图标
    QPixmap mainIcon = createAppIcon(QSize(64, 64));
    if (!saveBothFormats(mainIcon, baseName)) {
        allSuccess = false;
        qWarning() << "不能导出主要的 icon";
    }

    return allSuccess;
}

bool ImageManager::exportCustomIcon(const QPixmap &pixmap, const QString &baseName)
{
    if (pixmap.isNull()) {
        qWarning() << "不能导出空文件";
        return false;
    }

    return saveBothFormats(pixmap, baseName);
}

QString ImageManager::getImageDirPath() const
{
    return m_imageDir;
}

bool ImageManager::iconExists(const QString &baseName, const QString &format) const
{
    QString filePath = getIconPath(baseName, format);
    return QFile::exists(filePath);
}

QString ImageManager::getIconPath(const QString &baseName, const QString &format) const
{
    return QDir(m_imageDir).filePath(QString("%1.%2").arg(baseName, format));
}

QPixmap ImageManager::createAppIcon(const QSize &size)
{
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.setBrush(QBrush(QColor(52, 152, 139))); // #34988b
    painter.setPen(Qt::NoPen);

    // 根据尺寸调整圆角
    int cornerRadius = qMin(size.width(), size.height()) / 6.4;
    painter.drawRoundedRect(0, 0, size.width(), size.height(), cornerRadius, cornerRadius);

    // 绘制 emoji
    QFont font = getEmojiFont(size.width()  *0.5); // 字体大小为图标尺寸的50%
    painter.setFont(font);
    painter.setPen(QPen(Qt::white));

    // 使用双剑 emoji
    QString emoji = QString::fromUtf8(u8"⚔");
    painter.drawText(QRect(0, 0, size.width(), size.height()), Qt::AlignCenter, emoji);

    return pixmap;
}

bool ImageManager::ensureImageDirExists()
{
    QDir dir(m_imageDir);
    if (!dir.exists()) {
        if (dir.mkpath(".")) {
            qDebug() << "创建 images 文件夹成功:" << m_imageDir;
            return true;
        } else {
            qCritical() << "创建 images 文件夹失败:" << m_imageDir;
            return false;
        }
    }
    return true;
}

bool ImageManager::saveBothFormats(const QPixmap &pixmap, const QString &baseName)
{
    if (!ensureImageDirExists()) {
        return false;
    }

    bool success = true;

    // 保存 PNG
    QString pngPath = getIconPath(baseName, "png");
    if (!savePng(pixmap, pngPath)) {
        qWarning() << "保存 png 失败:" << pngPath;
        success = false;
    }

    // 保存 ICO（仅对 32x32 及以上尺寸）
    if (pixmap.width() >= 32 && pixmap.height() >= 32) {
        QString icoPath = getIconPath(baseName, "ico");
        if (!saveIco(pixmap, icoPath)) {
            qWarning() << "保存 ico 失败:" << icoPath;
            success = false;
        }
    }

    return success;
}

bool ImageManager::savePng(const QPixmap &pixmap, const QString &filePath)
{
    return pixmap.save(filePath, "PNG", 100); // 100 = 最高质量
}

bool ImageManager::saveIco(const QPixmap &pixmap, const QString &filePath)
{
    // Qt 对 ICO 格式的支持有限，这里使用简单的方法
    // 对于更好的 ICO 支持，可以考虑使用第三方库
    return pixmap.save(filePath, "ICO");
}

QFont ImageManager::getEmojiFont(int pixelSize)
{
    QFont font;

    // 尝试多种可能的 emoji 字体
    QStringList emojiFonts = {
        "Segoe UI Emoji",      // Windows
        "Apple Color Emoji",   // macOS
        "Noto Color Emoji",    // Linux
        "Android Emoji",       // Android
        "Arial",               // 回退
        "Microsoft Yahei"      // 中文系统回退
    };

    for (const QString &fontName : emojiFonts) {
        font.setFamily(fontName);
        font.setPixelSize(pixelSize);

        // 检查字体是否可用
        if (QFontInfo(font).family().contains(fontName, Qt::CaseInsensitive)) {
            qDebug() << "正在使用的 emoji 字体:" << fontName;
            break;
        }
    }

    return font;
}