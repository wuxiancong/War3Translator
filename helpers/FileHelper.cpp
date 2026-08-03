#include "FileHelper.h"
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QTcpSocket>
#include <QDataStream>
#include <QTextStream>
#include <QNetworkProxy>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QCryptographicHash>

#include <windows.h>
#include <fileapi.h>

FileHelper &FileHelper::instance()
{
    static FileHelper instance;
    return instance;
}

FileHelper::FileHelper(QObject *parent)
    : QObject(parent)
{

}

FileHelper::~FileHelper()
{
    // 清理资源
}

QString FileHelper::readFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    return in.readAll();
}

bool FileHelper::writeFile(const QString &filePath, const QString &content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << content;
    return true;
}

bool FileHelper::fileExists(const QString &filePath)
{
    return QFile::exists(filePath);
}

bool FileHelper::directoryExists(const QString &dirPath)
{
    return QDir(dirPath).exists();
}

bool FileHelper::createDirectory(const QString &dirPath)
{
    return QDir().mkpath(dirPath);
}

QString FileHelper::getAbsolutePath(const QString &path)
{
    return QFileInfo(path).absoluteFilePath();
}

QString FileHelper::getFileName(const QString &filePath)
{
    return QFileInfo(filePath).fileName();
}

QString FileHelper::getDirectoryPath(const QString &filePath)
{
    return QFileInfo(filePath).absolutePath();
}

qint64 FileHelper::getFileSize(const QString &filePath)
{
    return QFileInfo(filePath).size();
}

QString FileHelper::getFileModifiedTime(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.lastModified().toString("yyyy-MM-dd hh:mm:ss");
}

QStringList FileHelper::getFilesInDirectory(const QString &dirPath, const QString &filter)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return QStringList();
    }

    QStringList filters;
    if (!filter.isEmpty()) {
        filters << filter;
    }

    return dir.entryList(filters, QDir::Files | QDir::NoDotAndDotDot);
}

QStringList FileHelper::getDirectories(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return QStringList();
    }

    return dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
}

bool FileHelper::copyFile(const QString &sourcePath, const QString &destinationPath)
{
    return QFile::copy(sourcePath, destinationPath);
}

bool FileHelper::moveFile(const QString &sourcePath, const QString &destinationPath)
{
    return QFile::rename(sourcePath, destinationPath);
}

bool FileHelper::deleteFile(const QString &filePath)
{
    return QFile::remove(filePath);
}

bool FileHelper::uploadFile(const QString &filePath, const QString &host, quint16 port, const QString &crcHex, int timeout)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("无法打开文件: %1").arg(filePath));
        return false;
    }

    QString fileName = QFileInfo(filePath).fileName();
    qint64 fileSize = file.size();

    QTcpSocket socket;
    socket.setProxy(QNetworkProxy::NoProxy);

    qDebug() << "📤 连接服务器:" << host << ":" << port << "Token:" << crcHex;
    socket.connectToHost(host, port);

    if (!socket.waitForConnected(timeout)) {
        emit errorOccurred(QString("连接服务器失败: %1").arg(socket.errorString()));
        return false;
    }

    // 使用 QDataStream 构建头部数据
    QDataStream out(&socket);
    out.setVersion(QDataStream::Qt_5_15);

    // =========================================================
    // 🛡 发送安全协议头
    // 结构: [Magic 4] [CRC 8] [NameLen 4] [Name N] [Size 8]
    // =========================================================

    // 1. Magic Number (4 bytes) - 用于快速识别协议
    socket.write("W3UP", 4);

    // 2. CRC Token (8 bytes) - 用于服务端鉴权
    QByteArray tokenBytes = crcHex.toLatin1();
    if (tokenBytes.size() != 8) {
        // 简单的补全或截断逻辑，确保协议对齐
        tokenBytes = tokenBytes.leftJustified(8, '0');
    }
    socket.write(tokenBytes);

    // 3. 文件名长度 (4 bytes UINT32)
    QByteArray fileNameBytes = fileName.toUtf8();
    out << (quint32)fileNameBytes.size();

    // 4. 文件名数据
    socket.write(fileNameBytes);

    // 5. 文件大小 (8 bytes INT64)
    out << (qint64)fileSize;

    // 确保头部发送完毕
    if (!socket.waitForBytesWritten(timeout)) {
        emit errorOccurred("发送协议头超时");
        return false;
    }

    // =========================================================
    // 🚀 批量发送文件内容
    // =========================================================
    const qint64 CHUNK_SIZE = 64 * 1024; // 64KB
    qint64 bytesWritten = 0;
    char buffer[64 * 1024];

    while (!file.atEnd()) {
        qint64 len = file.read(buffer, CHUNK_SIZE);
        if (len > 0) {
            qint64 written = socket.write(buffer, len);

            // 等待写入缓冲区，防止内存暴涨
            if (!socket.waitForBytesWritten(timeout)) {
                emit errorOccurred("写入数据超时");
                return false;
            }

            bytesWritten += written;

            // 发送进度信号
            if (fileSize > 0) {
                int progress = static_cast<int>((bytesWritten * 100) / fileSize);
                emit uploadProgress(fileName, progress);
            }
        }
    }

    // 断开连接
    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.waitForDisconnected(timeout);
    }

    qDebug() << "✅ 传输完成:" << fileName;
    emit fileOperationCompleted("Upload: " + fileName, true);
    return true;
}

bool FileHelper::deleteDirectory(const QString &dirPath)
{
    QDir dir(dirPath);
    return dir.removeRecursively();
}

QString FileHelper::getApplicationDirPath()
{
    return QCoreApplication::applicationDirPath();
}

QString FileHelper::getDocumentsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString FileHelper::getTempPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation);
}

bool FileHelper::isAbsolutePath(const QString &path)
{
    return QFileInfo(path).isAbsolute();
}

QString FileHelper::combinePaths(const QString &path1, const QString &path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

FilePathInfo FileHelper::getFilePathInfo(const QString &filePath)
{
    return getFilePathInfo(filePath, QDir::currentPath());
}

FilePathInfo FileHelper::getFilePathInfo(const QString &filePath, const QString &baseDir)
{
    FilePathInfo info;
    QFileInfo fileInfo(filePath);

    info.absolutePath = fileInfo.absoluteFilePath();
    info.relativePath = calculateRelativePath(info.absolutePath, baseDir);
    info.fileName = fileInfo.fileName();
    info.baseName = fileInfo.baseName();
    info.suffix = fileInfo.suffix();
    info.directory = fileInfo.absolutePath();
    info.isValid = fileInfo.exists();

    return info;
}

QString FileHelper::getRelativePath(const QString &filePath)
{
    return getRelativePath(filePath, QDir::currentPath());
}

QString FileHelper::getRelativePath(const QString &filePath, const QString &baseDir)
{
    QFileInfo fileInfo(filePath);
    QString absolutePath = fileInfo.absoluteFilePath();
    return calculateRelativePath(absolutePath, baseDir);
}

QString FileHelper::calculateRelativePath(const QString &absolutePath, const QString &baseDir)
{
    QDir baseDirectory(baseDir);
    QString relativePath = baseDirectory.relativeFilePath(absolutePath);

    // 如果计算出的相对路径仍然是绝对路径，说明文件不在基准目录下
    if (QFileInfo(relativePath).isAbsolute()) {
        return absolutePath; // 返回绝对路径作为后备
    }

    return relativePath;
}

bool FileHelper::copyLoadDll(const QString &gamePath)
{
    // 首先进行合法性校验
    if (!isValidGameDirectory(gamePath)) {
        qDebug() << "❌ 非法的游戏路径，取消复制:" << gamePath;
        return false;
    }

    QString cleanPath = gamePath.trimmed();
    if (cleanPath.startsWith("file:///")) {
        cleanPath = cleanPath.mid(8);
    }
    QFileInfo gameFileInfo(cleanPath);
    QString gameDir = gameFileInfo.isDir() ? gameFileInfo.absoluteFilePath()
                                           : gameFileInfo.absolutePath();

    // 使用Qt API获取当前应用程序目录
    QString currentAppDir = QCoreApplication::applicationDirPath();
    QString sourceDllPath = currentAppDir + "/load.dll";
    QString targetDllPath = gameDir + "/load.dll";

    if (QString::compare(sourceDllPath, targetDllPath, Qt::CaseInsensitive) == 0) {
        qDebug() << "⚠ 源与目标相同，无需复制。";
        return true;
    }

    qDebug() << "源DLL路径:" << sourceDllPath;
    qDebug() << "目标DLL路径:" << targetDllPath;

    // 检查源文件是否存在
    QFile sourceFile(sourceDllPath);
    if (!sourceFile.exists()) {
        qDebug() << "源文件不存在:" << sourceDllPath;
        return false;
    }

    QFile targetFile(targetDllPath);

    // 如果目标文件已存在，先删除
    if (targetFile.exists()) {
        // 确保有写入权限
        targetFile.setPermissions(QFile::WriteOwner | QFile::ReadOwner);
        if (!targetFile.remove()) {
            qDebug() << "删除现有文件失败:" << targetDllPath;
            return false;
        }
    }

    // 复制文件
    if (!sourceFile.copy(targetDllPath)) {
        qDebug() << "复制文件失败:" << sourceFile.errorString();
        return false;
    }

    // 设置隐藏属性
    std::wstring targetWPath = targetDllPath.toStdWString();
    DWORD attributes = GetFileAttributesW(targetWPath.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        if (!SetFileAttributesW(targetWPath.c_str(), attributes | FILE_ATTRIBUTE_HIDDEN)) {
            qDebug() << "设置隐藏属性失败";
            // 不返回false，因为文件复制成功了
        } else {
            qDebug() << "成功设置隐藏属性";
        }
    }

    qDebug() << "DLL复制完成:" << targetDllPath;
    return true;
}

bool FileHelper::isFileOccupied(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists()) return false;

    // 尝试以读写模式打开。如果文件被占用，打开会失败。
    // 注意：QIODevice::ReadOnly 往往能成功，所以必须包含 Write 或使用独占标识
    if (file.open(QIODevice::ReadWrite)) {
        file.close();
        return false; // 能打开，说明没被占用
    }
    return true; // 打不开，说明被占用
}

bool FileHelper::isValidGameDirectory(const QString &path)
{
    // 1. 去除两端空格和换行
    QString cleanPath = path.trimmed();

    // 2. 基本非空检查
    if (cleanPath.isEmpty()) {
        return false;
    }

    // 3. 处理 URL 协议头 (file:///)
    if (cleanPath.startsWith("file:///")) {
        cleanPath = cleanPath.mid(8);
    } else if (cleanPath.startsWith("file://")) {
        cleanPath = cleanPath.mid(7);
    }

    // 4. 检查是否包含 Windows 路径常见的非法字符（除了冒号、斜杠、反斜杠）
    // 注意：这里可以根据需要放宽，但至少要排除一长串空格
    if (cleanPath.length() < 3 || !cleanPath.contains(":")) {
        return false;
    }

    // 5. 核心：使用 QDir 检查目录是否真实存在
    QDir dir(cleanPath);
    if (!dir.exists()) {
        // 如果输入的是文件路径，尝试获取其所在目录
        QFileInfo fileInfo(cleanPath);
        if (!fileInfo.absoluteDir().exists()) {
            return false;
        }
    }

    // 6. 安全防线：防止路径指向当前程序目录（防止自残）
    QString appDir = QDir(QCoreApplication::applicationDirPath()).absolutePath();
    QString targetDir = QDir(cleanPath).absolutePath();
    if (QString::compare(appDir, targetDir, Qt::CaseInsensitive) == 0) {
        qDebug() << "❌ 警告：检测到目标路径为程序运行目录，已拦截操作";
        return false;
    }

    return true;
}

bool FileHelper::isFileLockedNative(const QString &filePath)
{
    // 将路径转换为 Windows 宽字符格式
    std::wstring wPath = filePath.toStdWString();

    // 尝试以“零共享”模式打开。如果其他进程有任何句柄，这里都会失败。
    HANDLE hFile = CreateFileW(
        (LPCWSTR)wPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,                 // 0 表示不共享：如果有人占着，我绝对打不开
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE) {
        return true; // 文件被锁定
    }

    CloseHandle(hFile);
    return false; // 文件未被锁定
}

QByteArray FileHelper::getFileChecksum(const QString &fileName)
{
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        return QCryptographicHash::hash(f.readAll(), QCryptographicHash::Md5).toHex();
    }
    return QByteArray();
}
