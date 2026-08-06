/****************************************************************************
** Meta object code from reading C++ file 'IpcManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../managers/IpcManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'IpcManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_IpcManager_t {
    QByteArrayData data[11];
    char stringdata0[118];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_IpcManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_IpcManager_t qt_meta_stringdata_IpcManager = {
    {
QT_MOC_LITERAL(0, 0, 10), // "IpcManager"
QT_MOC_LITERAL(1, 11, 26), // "incomingMessageIntercepted"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 3), // "pid"
QT_MOC_LITERAL(4, 43, 6), // "sender"
QT_MOC_LITERAL(5, 50, 4), // "text"
QT_MOC_LITERAL(6, 55, 9), // "direction"
QT_MOC_LITERAL(7, 65, 20), // "onIpcMessageReceived"
QT_MOC_LITERAL(8, 86, 8), // "resetIpc"
QT_MOC_LITERAL(9, 95, 7), // "cleanup"
QT_MOC_LITERAL(10, 103, 14) // "initIpcManager"

    },
    "IpcManager\0incomingMessageIntercepted\0"
    "\0pid\0sender\0text\0direction\0"
    "onIpcMessageReceived\0resetIpc\0cleanup\0"
    "initIpcManager"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_IpcManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    4,   39,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    0,   48,    2, 0x0a /* Public */,

 // methods: name, argc, parameters, tag, flags
       8,    0,   49,    2, 0x02 /* Public */,
       9,    0,   50,    2, 0x02 /* Public */,
      10,    0,   51,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::UInt, QMetaType::QString, QMetaType::QString, QMetaType::UInt,    3,    4,    5,    6,

 // slots: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString,

       0        // eod
};

void IpcManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<IpcManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->incomingMessageIntercepted((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< quint32(*)>(_a[4]))); break;
        case 1: _t->onIpcMessageReceived(); break;
        case 2: _t->resetIpc(); break;
        case 3: _t->cleanup(); break;
        case 4: { QString _r = _t->initIpcManager();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (IpcManager::*)(quint32 , QString , QString , quint32 );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&IpcManager::incomingMessageIntercepted)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject IpcManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_IpcManager.data,
    qt_meta_data_IpcManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *IpcManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *IpcManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_IpcManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int IpcManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void IpcManager::incomingMessageIntercepted(quint32 _t1, QString _t2, QString _t3, quint32 _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
