#include "HardwareHelper.h"
#include <windows.h>
#include <comdef.h>
#include <WbemIdl.h>
#include <QDebug>

HardwareHelper& HardwareHelper::instance()
{
    static HardwareHelper instance;
    return instance;
}

HardwareHelper::HardwareHelper(QObject *parent)
    : QObject(parent)
{
}

HardwareHelper::~HardwareHelper()
{
}

QString HardwareHelper::queryWmi(const QString &wmiClass, const QString &field)
{
    QString result = "N/A";

    // 1. 初始化 COM
    HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);

    if (FAILED(hres) && hres != RPC_E_CHANGED_MODE) {
        return result;
    }

    // 2. 设置 COM 安全级别
    hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                                RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    (void)hres;

    IWbemLocator *pLoc = NULL;
    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (SUCCEEDED(hres)) {
        IWbemServices *pSvc = NULL;
        BSTR bstrNamespace = SysAllocString(L"ROOT\\CIMV2");
        hres = pLoc->ConnectServer(bstrNamespace, nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
        SysFreeString(bstrNamespace);

        if (SUCCEEDED(hres)) {
            hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                     RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            (void)hres;

            IEnumWbemClassObject* pEnumerator = NULL;
            QString query = QString("SELECT %1 FROM %2").arg(field, wmiClass);
            BSTR bstrWQL = SysAllocString(L"WQL");
            BSTR bstrQuery = SysAllocString((const OLECHAR*)query.utf16());

            hres = pSvc->ExecQuery(bstrWQL, bstrQuery,
                                   WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                   NULL, &pEnumerator);

            SysFreeString(bstrWQL);
            SysFreeString(bstrQuery);

            if (SUCCEEDED(hres) && pEnumerator) {
                IWbemClassObject *pclsObj = NULL;
                ULONG uReturn = 0;
                pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

                if (uReturn != 0) {
                    VARIANT vtProp;
                    hres = pclsObj->Get(field.toStdWString().c_str(), 0, &vtProp, 0, 0);
                    if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
                        result = QString::fromWCharArray(vtProp.bstrVal).trimmed();
                    }
                    VariantClear(&vtProp);
                    pclsObj->Release();
                }
                pEnumerator->Release();
            }
            pSvc->Release();
        }
        pLoc->Release();
    }

    CoUninitialize();

    if (result.isEmpty() || result.contains("To be filled", Qt::CaseInsensitive)) {
        return "N/A";
    }
    return result;
}

QString HardwareHelper::motherboardSerial() {
    if (m_cachedMb.isEmpty()) m_cachedMb = queryWmi("Win32_BaseBoard", "SerialNumber");
    return m_cachedMb;
}

QString HardwareHelper::cpuId() {
    if (m_cachedCpu.isEmpty()) m_cachedCpu = queryWmi("Win32_Processor", "ProcessorId");
    return m_cachedCpu;
}

QString HardwareHelper::diskId() {
    if (m_cachedDisk.isEmpty()) m_cachedDisk = queryWmi("Win32_PhysicalMedia", "SerialNumber");
    return m_cachedDisk;
}

QString HardwareHelper::generateHardwareID(const QString &salt)
{
    if (!m_cachedHwid.isEmpty()) return m_cachedHwid;

    QString mb = motherboardSerial();
    QString cpu = cpuId();
    QString disk = diskId();

    // 组合硬件信息
    QString rawKey = QString("%1|%2|%3|%4").arg(salt, mb, cpu, disk);

    // 生成 SHA-256 哈希
    QByteArray hash = QCryptographicHash::hash(rawKey.toUtf8(), QCryptographicHash::Sha256);
    m_cachedHwid = hash.toHex().toUpper();

    qDebug() << "🛡 [HardwareID] Generated:" << m_cachedHwid;
    return m_cachedHwid;
}
