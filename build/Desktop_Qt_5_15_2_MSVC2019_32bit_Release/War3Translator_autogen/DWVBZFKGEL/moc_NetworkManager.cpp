/****************************************************************************
** Meta object code from reading C++ file 'NetworkManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../managers/NetworkManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NetworkManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NetworkManager_t {
    QByteArrayData data[61];
    char stringdata0[908];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NetworkManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NetworkManager_t qt_meta_stringdata_NetworkManager = {
    {
QT_MOC_LITERAL(0, 0, 14), // "NetworkManager"
QT_MOC_LITERAL(1, 15, 19), // "networkDisconnected"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 22), // "unhostResponseReceived"
QT_MOC_LITERAL(4, 59, 25), // "unregisteredStateDetected"
QT_MOC_LITERAL(5, 85, 10), // "botChanged"
QT_MOC_LITERAL(6, 96, 10), // "ServerName"
QT_MOC_LITERAL(7, 107, 10), // "serverName"
QT_MOC_LITERAL(8, 118, 16), // "tcpStatusChanged"
QT_MOC_LITERAL(9, 135, 9), // "connected"
QT_MOC_LITERAL(10, 145, 10), // "statusText"
QT_MOC_LITERAL(11, 156, 12), // "dataReceived"
QT_MOC_LITERAL(12, 169, 4), // "data"
QT_MOC_LITERAL(13, 174, 12), // "QHostAddress"
QT_MOC_LITERAL(14, 187, 8), // "fromAddr"
QT_MOC_LITERAL(15, 196, 8), // "fromPort"
QT_MOC_LITERAL(16, 205, 26), // "udpConnectionStatusChanged"
QT_MOC_LITERAL(17, 232, 17), // "RegistrationState"
QT_MOC_LITERAL(18, 250, 17), // "registrationState"
QT_MOC_LITERAL(19, 268, 12), // "networkError"
QT_MOC_LITERAL(20, 281, 5), // "error"
QT_MOC_LITERAL(21, 287, 18), // "registrationStatus"
QT_MOC_LITERAL(22, 306, 7), // "success"
QT_MOC_LITERAL(23, 314, 7), // "details"
QT_MOC_LITERAL(24, 322, 20), // "unregistrationStatus"
QT_MOC_LITERAL(25, 343, 21), // "onTcpHeartbeatTimeout"
QT_MOC_LITERAL(26, 365, 21), // "onUdpHeartbeatTimeout"
QT_MOC_LITERAL(27, 387, 21), // "onRegistrationTimeout"
QT_MOC_LITERAL(28, 409, 14), // "onTcpConnected"
QT_MOC_LITERAL(29, 424, 14), // "onTcpReadyRead"
QT_MOC_LITERAL(30, 439, 17), // "onTcpDisconnected"
QT_MOC_LITERAL(31, 457, 10), // "onTcpError"
QT_MOC_LITERAL(32, 468, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(33, 497, 11), // "socketError"
QT_MOC_LITERAL(34, 509, 17), // "onTcpStateChanged"
QT_MOC_LITERAL(35, 527, 28), // "QAbstractSocket::SocketState"
QT_MOC_LITERAL(36, 556, 11), // "socketState"
QT_MOC_LITERAL(37, 568, 27), // "onUnregisteredStateDetected"
QT_MOC_LITERAL(38, 596, 28), // "onUdpConnectionStatusChanged"
QT_MOC_LITERAL(39, 625, 23), // "onPublicSocketReadyRead"
QT_MOC_LITERAL(40, 649, 13), // "onSocketError"
QT_MOC_LITERAL(41, 663, 20), // "onSocketStateChanged"
QT_MOC_LITERAL(42, 684, 5), // "state"
QT_MOC_LITERAL(43, 690, 21), // "onTranslationFinished"
QT_MOC_LITERAL(44, 712, 5), // "msgId"
QT_MOC_LITERAL(45, 718, 3), // "pid"
QT_MOC_LITERAL(46, 722, 4), // "flag"
QT_MOC_LITERAL(47, 727, 10), // "extraScope"
QT_MOC_LITERAL(48, 738, 9), // "direction"
QT_MOC_LITERAL(49, 748, 15), // "originalMessage"
QT_MOC_LITERAL(50, 764, 17), // "translatedMessage"
QT_MOC_LITERAL(51, 782, 8), // "language"
QT_MOC_LITERAL(52, 791, 13), // "stopListening"
QT_MOC_LITERAL(53, 805, 12), // "gracefulExit"
QT_MOC_LITERAL(54, 818, 17), // "handleChatCommand"
QT_MOC_LITERAL(55, 836, 8), // "fullText"
QT_MOC_LITERAL(56, 845, 8), // "userName"
QT_MOC_LITERAL(57, 854, 18), // "sendCommandMessage"
QT_MOC_LITERAL(58, 873, 7), // "command"
QT_MOC_LITERAL(59, 881, 4), // "text"
QT_MOC_LITERAL(60, 886, 21) // "sendTranslatedMessage"

    },
    "NetworkManager\0networkDisconnected\0\0"
    "unhostResponseReceived\0unregisteredStateDetected\0"
    "botChanged\0ServerName\0serverName\0"
    "tcpStatusChanged\0connected\0statusText\0"
    "dataReceived\0data\0QHostAddress\0fromAddr\0"
    "fromPort\0udpConnectionStatusChanged\0"
    "RegistrationState\0registrationState\0"
    "networkError\0error\0registrationStatus\0"
    "success\0details\0unregistrationStatus\0"
    "onTcpHeartbeatTimeout\0onUdpHeartbeatTimeout\0"
    "onRegistrationTimeout\0onTcpConnected\0"
    "onTcpReadyRead\0onTcpDisconnected\0"
    "onTcpError\0QAbstractSocket::SocketError\0"
    "socketError\0onTcpStateChanged\0"
    "QAbstractSocket::SocketState\0socketState\0"
    "onUnregisteredStateDetected\0"
    "onUdpConnectionStatusChanged\0"
    "onPublicSocketReadyRead\0onSocketError\0"
    "onSocketStateChanged\0state\0"
    "onTranslationFinished\0msgId\0pid\0flag\0"
    "extraScope\0direction\0originalMessage\0"
    "translatedMessage\0language\0stopListening\0"
    "gracefulExit\0handleChatCommand\0fullText\0"
    "userName\0sendCommandMessage\0command\0"
    "text\0sendTranslatedMessage"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NetworkManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      29,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  159,    2, 0x06 /* Public */,
       3,    0,  160,    2, 0x06 /* Public */,
       4,    0,  161,    2, 0x06 /* Public */,
       5,    1,  162,    2, 0x06 /* Public */,
       8,    2,  165,    2, 0x06 /* Public */,
      11,    3,  170,    2, 0x06 /* Public */,
      16,    1,  177,    2, 0x06 /* Public */,
      19,    1,  180,    2, 0x06 /* Public */,
      21,    2,  183,    2, 0x06 /* Public */,
      24,    1,  188,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      25,    0,  191,    2, 0x08 /* Private */,
      26,    0,  192,    2, 0x08 /* Private */,
      27,    0,  193,    2, 0x08 /* Private */,
      28,    0,  194,    2, 0x08 /* Private */,
      29,    0,  195,    2, 0x08 /* Private */,
      30,    0,  196,    2, 0x08 /* Private */,
      31,    1,  197,    2, 0x08 /* Private */,
      34,    1,  200,    2, 0x08 /* Private */,
      37,    0,  203,    2, 0x08 /* Private */,
      38,    1,  204,    2, 0x08 /* Private */,
      39,    0,  207,    2, 0x08 /* Private */,
      40,    1,  208,    2, 0x08 /* Private */,
      41,    1,  211,    2, 0x08 /* Private */,
      43,    8,  214,    2, 0x08 /* Private */,

 // methods: name, argc, parameters, tag, flags
      52,    0,  231,    2, 0x02 /* Public */,
      53,    0,  232,    2, 0x02 /* Public */,
      54,    2,  233,    2, 0x02 /* Public */,
      57,    3,  238,    2, 0x02 /* Public */,
      60,    4,  245,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::QByteArray, 0x80000000 | 13, QMetaType::UShort,   12,   14,   15,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, QMetaType::QString,   20,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   22,   23,
    QMetaType::Void, QMetaType::Bool,   22,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 32,   33,
    QMetaType::Void, 0x80000000 | 35,   36,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 32,   20,
    QMetaType::Void, 0x80000000 | 35,   42,
    QMetaType::Void, QMetaType::ULongLong, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt, QMetaType::QString, QMetaType::QString, QMetaType::QString,   44,   45,   46,   47,   48,   49,   50,   51,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   55,   56,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   56,   58,   59,
    QMetaType::Void, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt, QMetaType::QString,   45,   46,   47,   50,

       0        // eod
};

void NetworkManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->networkDisconnected(); break;
        case 1: _t->unhostResponseReceived(); break;
        case 2: _t->unregisteredStateDetected(); break;
        case 3: _t->botChanged((*reinterpret_cast< ServerName(*)>(_a[1]))); break;
        case 4: _t->tcpStatusChanged((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 5: _t->dataReceived((*reinterpret_cast< const QByteArray(*)>(_a[1])),(*reinterpret_cast< const QHostAddress(*)>(_a[2])),(*reinterpret_cast< quint16(*)>(_a[3]))); break;
        case 6: _t->udpConnectionStatusChanged((*reinterpret_cast< RegistrationState(*)>(_a[1]))); break;
        case 7: _t->networkError((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->registrationStatus((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->unregistrationStatus((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->onTcpHeartbeatTimeout(); break;
        case 11: _t->onUdpHeartbeatTimeout(); break;
        case 12: _t->onRegistrationTimeout(); break;
        case 13: _t->onTcpConnected(); break;
        case 14: _t->onTcpReadyRead(); break;
        case 15: _t->onTcpDisconnected(); break;
        case 16: _t->onTcpError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 17: _t->onTcpStateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 18: _t->onUnregisteredStateDetected(); break;
        case 19: _t->onUdpConnectionStatusChanged((*reinterpret_cast< RegistrationState(*)>(_a[1]))); break;
        case 20: _t->onPublicSocketReadyRead(); break;
        case 21: _t->onSocketError((*reinterpret_cast< QAbstractSocket::SocketError(*)>(_a[1]))); break;
        case 22: _t->onSocketStateChanged((*reinterpret_cast< QAbstractSocket::SocketState(*)>(_a[1]))); break;
        case 23: _t->onTranslationFinished((*reinterpret_cast< quint64(*)>(_a[1])),(*reinterpret_cast< quint32(*)>(_a[2])),(*reinterpret_cast< quint32(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4])),(*reinterpret_cast< quint32(*)>(_a[5])),(*reinterpret_cast< QString(*)>(_a[6])),(*reinterpret_cast< QString(*)>(_a[7])),(*reinterpret_cast< QString(*)>(_a[8]))); break;
        case 24: _t->stopListening(); break;
        case 25: _t->gracefulExit(); break;
        case 26: _t->handleChatCommand((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 27: _t->sendCommandMessage((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 28: _t->sendTranslatedMessage((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint32(*)>(_a[2])),(*reinterpret_cast< quint32(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 16:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 17:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketState >(); break;
            }
            break;
        case 21:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 22:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QAbstractSocket::SocketState >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NetworkManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::networkDisconnected)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::unhostResponseReceived)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::unregisteredStateDetected)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(ServerName );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::botChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::tcpStatusChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(const QByteArray & , const QHostAddress & , quint16 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::dataReceived)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(RegistrationState );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::udpConnectionStatusChanged)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::networkError)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::registrationStatus)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (NetworkManager::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NetworkManager::unregistrationStatus)) {
                *result = 9;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NetworkManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_NetworkManager.data,
    qt_meta_data_NetworkManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NetworkManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NetworkManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    return _id;
}

// SIGNAL 0
void NetworkManager::networkDisconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NetworkManager::unhostResponseReceived()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkManager::unregisteredStateDetected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NetworkManager::botChanged(ServerName _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkManager::tcpStatusChanged(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NetworkManager::dataReceived(const QByteArray & _t1, const QHostAddress & _t2, quint16 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void NetworkManager::udpConnectionStatusChanged(RegistrationState _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void NetworkManager::networkError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void NetworkManager::registrationStatus(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void NetworkManager::unregistrationStatus(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
