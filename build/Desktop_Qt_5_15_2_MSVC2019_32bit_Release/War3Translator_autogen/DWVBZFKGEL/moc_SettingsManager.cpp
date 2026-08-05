/****************************************************************************
** Meta object code from reading C++ file 'SettingsManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../managers/SettingsManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SettingsManager_t {
    QByteArrayData data[21];
    char stringdata0[292];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SettingsManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SettingsManager_t qt_meta_stringdata_SettingsManager = {
    {
QT_MOC_LITERAL(0, 0, 15), // "SettingsManager"
QT_MOC_LITERAL(1, 16, 19), // "languageCodeChanged"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 21), // "languageAboutToChange"
QT_MOC_LITERAL(4, 59, 24), // "translateLanguageChanged"
QT_MOC_LITERAL(5, 84, 16), // "isLoadingChanged"
QT_MOC_LITERAL(6, 101, 15), // "setLanguageCode"
QT_MOC_LITERAL(7, 117, 4), // "code"
QT_MOC_LITERAL(8, 122, 20), // "setTranslateLanguage"
QT_MOC_LITERAL(9, 143, 21), // "isDefaultShoutContent"
QT_MOC_LITERAL(10, 165, 7), // "content"
QT_MOC_LITERAL(11, 173, 19), // "isDefaultSchemeName"
QT_MOC_LITERAL(12, 193, 4), // "name"
QT_MOC_LITERAL(13, 198, 15), // "serverAddresses"
QT_MOC_LITERAL(14, 214, 5), // "index"
QT_MOC_LITERAL(15, 220, 10), // "serverPort"
QT_MOC_LITERAL(16, 231, 10), // "hardwareId"
QT_MOC_LITERAL(17, 242, 8), // "clientId"
QT_MOC_LITERAL(18, 251, 17), // "translateLanguage"
QT_MOC_LITERAL(19, 269, 12), // "languageCode"
QT_MOC_LITERAL(20, 282, 9) // "isLoading"

    },
    "SettingsManager\0languageCodeChanged\0"
    "\0languageAboutToChange\0translateLanguageChanged\0"
    "isLoadingChanged\0setLanguageCode\0code\0"
    "setTranslateLanguage\0isDefaultShoutContent\0"
    "content\0isDefaultSchemeName\0name\0"
    "serverAddresses\0index\0serverPort\0"
    "hardwareId\0clientId\0translateLanguage\0"
    "languageCode\0isLoading"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       3,  102, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x06 /* Public */,
       3,    0,   80,    2, 0x06 /* Public */,
       4,    0,   81,    2, 0x06 /* Public */,
       5,    0,   82,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       6,    1,   83,    2, 0x02 /* Public */,
       8,    1,   86,    2, 0x02 /* Public */,
       9,    1,   89,    2, 0x02 /* Public */,
      11,    1,   92,    2, 0x02 /* Public */,
      13,    1,   95,    2, 0x02 /* Public */,
      13,    0,   98,    2, 0x22 /* Public | MethodCloned */,
      15,    0,   99,    2, 0x02 /* Public */,
      16,    0,  100,    2, 0x02 /* Public */,
      17,    0,  101,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Bool, QMetaType::QString,   10,
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::QString, QMetaType::UInt,   14,
    QMetaType::QString,
    QMetaType::UShort,
    QMetaType::QString,
    QMetaType::QString,

 // properties: name, type, flags
      18, QMetaType::QString, 0x00495103,
      19, QMetaType::QString, 0x00495103,
      20, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       2,
       0,
       3,

       0        // eod
};

void SettingsManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->languageCodeChanged(); break;
        case 1: _t->languageAboutToChange(); break;
        case 2: _t->translateLanguageChanged(); break;
        case 3: _t->isLoadingChanged(); break;
        case 4: _t->setLanguageCode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->setTranslateLanguage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: { bool _r = _t->isDefaultShoutContent((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->isDefaultSchemeName((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { QString _r = _t->serverAddresses((*reinterpret_cast< quint32(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 9: { QString _r = _t->serverAddresses();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 10: { quint16 _r = _t->serverPort();
            if (_a[0]) *reinterpret_cast< quint16*>(_a[0]) = std::move(_r); }  break;
        case 11: { QString _r = _t->hardwareId();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { QString _r = _t->clientId();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::languageCodeChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::languageAboutToChange)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::translateLanguageChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::isLoadingChanged)) {
                *result = 3;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->translateLanguage(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->languageCode(); break;
        case 2: *reinterpret_cast< bool*>(_v) = _t->isLoading(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setTranslateLanguage(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setLanguageCode(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject SettingsManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SettingsManager.data,
    qt_meta_data_SettingsManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SettingsManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SettingsManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SettingsManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void SettingsManager::languageCodeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsManager::languageAboutToChange()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsManager::translateLanguageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsManager::isLoadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
