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
    QByteArrayData data[26];
    char stringdata0[385];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SettingsManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SettingsManager_t qt_meta_stringdata_SettingsManager = {
    {
QT_MOC_LITERAL(0, 0, 15), // "SettingsManager"
QT_MOC_LITERAL(1, 16, 16), // "isLoadingChanged"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 19), // "languageCodeChanged"
QT_MOC_LITERAL(4, 54, 21), // "languageAboutToChange"
QT_MOC_LITERAL(5, 76, 24), // "translateLanguageChanged"
QT_MOC_LITERAL(6, 101, 25), // "translateLanguagesChanged"
QT_MOC_LITERAL(7, 127, 15), // "getLanguageName"
QT_MOC_LITERAL(8, 143, 4), // "code"
QT_MOC_LITERAL(9, 148, 15), // "setLanguageCode"
QT_MOC_LITERAL(10, 164, 20), // "setTranslateLanguage"
QT_MOC_LITERAL(11, 185, 21), // "setTranslateLanguages"
QT_MOC_LITERAL(12, 207, 9), // "languages"
QT_MOC_LITERAL(13, 217, 21), // "isDefaultShoutContent"
QT_MOC_LITERAL(14, 239, 7), // "content"
QT_MOC_LITERAL(15, 247, 19), // "isDefaultSchemeName"
QT_MOC_LITERAL(16, 267, 4), // "name"
QT_MOC_LITERAL(17, 272, 15), // "serverAddresses"
QT_MOC_LITERAL(18, 288, 5), // "index"
QT_MOC_LITERAL(19, 294, 10), // "serverPort"
QT_MOC_LITERAL(20, 305, 10), // "hardwareId"
QT_MOC_LITERAL(21, 316, 8), // "clientId"
QT_MOC_LITERAL(22, 325, 18), // "translateLanguages"
QT_MOC_LITERAL(23, 344, 17), // "translateLanguage"
QT_MOC_LITERAL(24, 362, 12), // "languageCode"
QT_MOC_LITERAL(25, 375, 9) // "isLoading"

    },
    "SettingsManager\0isLoadingChanged\0\0"
    "languageCodeChanged\0languageAboutToChange\0"
    "translateLanguageChanged\0"
    "translateLanguagesChanged\0getLanguageName\0"
    "code\0setLanguageCode\0setTranslateLanguage\0"
    "setTranslateLanguages\0languages\0"
    "isDefaultShoutContent\0content\0"
    "isDefaultSchemeName\0name\0serverAddresses\0"
    "index\0serverPort\0hardwareId\0clientId\0"
    "translateLanguages\0translateLanguage\0"
    "languageCode\0isLoading"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       4,  124, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x06 /* Public */,
       3,    0,   95,    2, 0x06 /* Public */,
       4,    0,   96,    2, 0x06 /* Public */,
       5,    0,   97,    2, 0x06 /* Public */,
       6,    0,   98,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       7,    1,   99,    2, 0x02 /* Public */,
       9,    1,  102,    2, 0x02 /* Public */,
      10,    1,  105,    2, 0x02 /* Public */,
      11,    1,  108,    2, 0x02 /* Public */,
      13,    1,  111,    2, 0x02 /* Public */,
      15,    1,  114,    2, 0x02 /* Public */,
      17,    1,  117,    2, 0x02 /* Public */,
      17,    0,  120,    2, 0x22 /* Public | MethodCloned */,
      19,    0,  121,    2, 0x02 /* Public */,
      20,    0,  122,    2, 0x02 /* Public */,
      21,    0,  123,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::QString, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QStringList,   12,
    QMetaType::Bool, QMetaType::QString,   14,
    QMetaType::Bool, QMetaType::QString,   16,
    QMetaType::QString, QMetaType::UInt,   18,
    QMetaType::QString,
    QMetaType::UShort,
    QMetaType::QString,
    QMetaType::QString,

 // properties: name, type, flags
      22, QMetaType::QStringList, 0x00495103,
      23, QMetaType::QString, 0x00495103,
      24, QMetaType::QString, 0x00495103,
      25, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       4,
       3,
       1,
       0,

       0        // eod
};

void SettingsManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->isLoadingChanged(); break;
        case 1: _t->languageCodeChanged(); break;
        case 2: _t->languageAboutToChange(); break;
        case 3: _t->translateLanguageChanged(); break;
        case 4: _t->translateLanguagesChanged(); break;
        case 5: { QString _r = _t->getLanguageName((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->setLanguageCode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->setTranslateLanguage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->setTranslateLanguages((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 9: { bool _r = _t->isDefaultShoutContent((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->isDefaultSchemeName((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { QString _r = _t->serverAddresses((*reinterpret_cast< quint32(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { QString _r = _t->serverAddresses();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 13: { quint16 _r = _t->serverPort();
            if (_a[0]) *reinterpret_cast< quint16*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->hardwareId();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { QString _r = _t->clientId();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::isLoadingChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::languageCodeChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::languageAboutToChange)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::translateLanguageChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::translateLanguagesChanged)) {
                *result = 4;
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
        case 0: *reinterpret_cast< QStringList*>(_v) = _t->translateLanguages(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->translateLanguage(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->languageCode(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->isLoading(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setTranslateLanguages(*reinterpret_cast< QStringList*>(_v)); break;
        case 1: _t->setTranslateLanguage(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setLanguageCode(*reinterpret_cast< QString*>(_v)); break;
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
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 4;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 4;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void SettingsManager::isLoadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsManager::languageCodeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsManager::languageAboutToChange()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsManager::translateLanguageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsManager::translateLanguagesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
