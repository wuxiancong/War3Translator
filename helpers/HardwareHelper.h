#ifndef HARDWAREHELPER_H
#define HARDWAREHELPER_H

#include <QObject>
#include <QString>
#include <QCryptographicHash>

class HardwareHelper : public QObject
{
    Q_OBJECT

public:
    // 禁用拷贝和赋值
    HardwareHelper(const HardwareHelper&) = delete;
    HardwareHelper& operator=(const HardwareHelper&) = delete;

    // 获取单例实例
    static HardwareHelper &instance();

    // 生成硬件指纹
    QString generateHardwareID(const QString &salt = "CC_War3_Platform_Salt");

    // 获取具体硬件信息
    QString motherboardSerial();
    QString cpuId();
    QString diskId();

private:
    explicit HardwareHelper(QObject *parent = nullptr);
    ~HardwareHelper();

    // 通用 WMI 查询
    QString queryWmi(const QString &wmiClass, const QString &field);

    // 缓存变量，避免重复查询
    QString m_cachedMb;
    QString m_cachedCpu;
    QString m_cachedDisk;
    QString m_cachedHwid;
};

#endif // HARDWAREHELPER_H
