#ifndef FILEHELPER_H
#define FILEHELPER_H

#include <QObject>
#include <QString>

// 文件路径信息结构体
struct FilePathInfo {
    QString absolutePath;    // 绝对路径
    QString relativePath;    // 相对路径
    QString fileName;        // 文件名（含扩展名）
    QString baseName;        // 文件名（不含扩展名）
    QString suffix;          // 文件扩展名
    QString directory;       // 所在目录
    bool isValid;           // 是否有效
};

class FileHelper : public QObject
{
    Q_OBJECT

public:
    // 禁止拷贝和赋值
    FileHelper(const FileHelper&) = delete;
    FileHelper& operator=(const FileHelper&) = delete;

    // 获取单例实例
    static FileHelper &instance();

    // 基本文件操作
    QString readFile(const QString &filePath);
    bool writeFile(const QString &filePath, const QString &content);
    bool fileExists(const QString &filePath);
    bool directoryExists(const QString &dirPath);
    bool createDirectory(const QString &dirPath);

    // 路径操作
    QString getAbsolutePath(const QString &path);
    QString getFileName(const QString &filePath);
    QString getDirectoryPath(const QString &filePath);

    // 文件信息
    qint64 getFileSize(const QString &filePath);
    QByteArray getFileChecksum(const QString &fileName);
    QString getFileModifiedTime(const QString &filePath);

    // 目录操作
    QStringList getFilesInDirectory(const QString &dirPath, const QString &filter = "");
    QStringList getDirectories(const QString &dirPath);

    // 文件操作
    bool copyFile(const QString &sourcePath, const QString &destinationPath);
    bool moveFile(const QString &sourcePath, const QString &destinationPath);
    bool deleteDirectory(const QString &dirPath);
    bool deleteFile(const QString &filePath);
    bool copyLoadDll(const QString &gamePath);
    bool isFileOccupied(const QString &filePath);
    bool isValidGameDirectory(const QString &path);
    bool isFileLockedNative(const QString &filePath);

    // 路径工具
    QString getApplicationDirPath();
    QString getDocumentsPath();
    QString getTempPath();
    bool isAbsolutePath(const QString &path);
    QString combinePaths(const QString &path1, const QString &path2);

    // 获取文件路径信息
    FilePathInfo getFilePathInfo(const QString &filePath);
    FilePathInfo getFilePathInfo(const QString &filePath, const QString &baseDir);

    // 获取相对路径（重载版本）
    QString getRelativePath(const QString &filePath);
    QString getRelativePath(const QString &filePath, const QString &baseDir);

    // 上传文件
    bool uploadFile(const QString &filePath, const QString &host, quint16 port, const QString &crcHex, int timeout = 30000);

signals:
    void fileOperationCompleted(const QString &operation, bool success);
    void uploadProgress(const QString &fileName, int percentage);
    void errorOccurred(const QString &errorMessage);

private:
    // 构造函数私有化
    explicit FileHelper(QObject *parent = nullptr);
    ~FileHelper();

    QString calculateRelativePath(const QString &absolutePath, const QString &baseDir);
};

#endif // FILEHELPER_H
