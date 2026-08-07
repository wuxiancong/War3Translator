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
    QByteArrayData data[30];
    char stringdata0[470];
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
QT_MOC_LITERAL(7, 127, 28), // "translateSendIntervalChanged"
QT_MOC_LITERAL(8, 156, 15), // "getLanguageName"
QT_MOC_LITERAL(9, 172, 4), // "code"
QT_MOC_LITERAL(10, 177, 15), // "setLanguageCode"
QT_MOC_LITERAL(11, 193, 20), // "setTranslateLanguage"
QT_MOC_LITERAL(12, 214, 21), // "setTranslateLanguages"
QT_MOC_LITERAL(13, 236, 9), // "languages"
QT_MOC_LITERAL(14, 246, 24), // "setTranslateSendInterval"
QT_MOC_LITERAL(15, 271, 8), // "interval"
QT_MOC_LITERAL(16, 280, 21), // "isDefaultShoutContent"
QT_MOC_LITERAL(17, 302, 7), // "content"
QT_MOC_LITERAL(18, 310, 19), // "isDefaultSchemeName"
QT_MOC_LITERAL(19, 330, 4), // "name"
QT_MOC_LITERAL(20, 335, 15), // "serverAddresses"
QT_MOC_LITERAL(21, 351, 5), // "index"
QT_MOC_LITERAL(22, 357, 10), // "serverPort"
QT_MOC_LITERAL(23, 368, 10), // "hardwareId"
QT_MOC_LITERAL(24, 379, 8), // "clientId"
QT_MOC_LITERAL(25, 388, 21), // "translateSendInterval"
QT_MOC_LITERAL(26, 410, 18), // "translateLanguages"
QT_MOC_LITERAL(27, 429, 17), // "translateLanguage"
QT_MOC_LITERAL(28, 447, 12), // "languageCode"
QT_MOC_LITERAL(29, 460, 9) // "isLoading"

    },
    "SettingsManager\0isLoadingChanged\0\0"
    "languageCodeChanged\0languageAboutToChange\0"
    "translateLanguageChanged\0"
    "translateLanguagesChanged\0"
    "translateSendIntervalChanged\0"
    "getLanguageName\0code\0setLanguageCode\0"
    "setTranslateLanguage\0setTranslateLanguages\0"
    "languages\0setTranslateSendInterval\0"
    "interval\0isDefaultShoutContent\0content\0"
    "isDefaultSchemeName\0name\0serverAddresses\0"
    "index\0serverPort\0hardwareId\0clientId\0"
    "translateSendInterval\0translateLanguages\0"
    "translateLanguage\0languageCode\0isLoading"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SettingsManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       5,  138, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,  104,    2, 0x06 /* Public */,
       3,    0,  105,    2, 0x06 /* Public */,
       4,    0,  106,    2, 0x06 /* Public */,
       5,    0,  107,    2, 0x06 /* Public */,
       6,    0,  108,    2, 0x06 /* Public */,
       7,    0,  109,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
       8,    1,  110,    2, 0x02 /* Public */,
      10,    1,  113,    2, 0x02 /* Public */,
      11,    1,  116,    2, 0x02 /* Public */,
      12,    1,  119,    2, 0x02 /* Public */,
      14,    1,  122,    2, 0x02 /* Public */,
      16,    1,  125,    2, 0x02 /* Public */,
      18,    1,  128,    2, 0x02 /* Public */,
      20,    1,  131,    2, 0x02 /* Public */,
      20,    0,  134,    2, 0x22 /* Public | MethodCloned */,
      22,    0,  135,    2, 0x02 /* Public */,
      23,    0,  136,    2, 0x02 /* Public */,
      24,    0,  137,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::QString, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QStringList,   13,
    QMetaType::Void, QMetaType::UInt,   15,
    QMetaType::Bool, QMetaType::QString,   17,
    QMetaType::Bool, QMetaType::QString,   19,
    QMetaType::QString, QMetaType::UInt,   21,
    QMetaType::QString,
    QMetaType::UShort,
    QMetaType::QString,
    QMetaType::QString,

 // properties: name, type, flags
      25, QMetaType::UInt, 0x00495103,
      26, QMetaType::QStringList, 0x00495103,
      27, QMetaType::QString, 0x00495103,
      28, QMetaType::QString, 0x00495103,
      29, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       5,
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
        case 5: _t->translateSendIntervalChanged(); break;
        case 6: { QString _r = _t->getLanguageName((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->setLanguageCode((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->setTranslateLanguage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->setTranslateLanguages((*reinterpret_cast< const QStringList(*)>(_a[1]))); break;
        case 10: _t->setTranslateSendInterval((*reinterpret_cast< quint32(*)>(_a[1]))); break;
        case 11: { bool _r = _t->isDefaultShoutContent((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 12: { bool _r = _t->isDefaultSchemeName((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 13: { QString _r = _t->serverAddresses((*reinterpret_cast< quint32(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->serverAddresses();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { quint16 _r = _t->serverPort();
            if (_a[0]) *reinterpret_cast< quint16*>(_a[0]) = std::move(_r); }  break;
        case 16: { QString _r = _t->hardwareId();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 17: { QString _r = _t->clientId();
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
        {
            using _t = void (SettingsManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SettingsManager::translateSendIntervalChanged)) {
                *result = 5;
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
        case 0: *reinterpret_cast< quint32*>(_v) = _t->translateSendInterval(); break;
        case 1: *reinterpret_cast< QStringList*>(_v) = _t->translateLanguages(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->translateLanguage(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->languageCode(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->isLoading(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SettingsManager *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setTranslateSendInterval(*reinterpret_cast< quint32*>(_v)); break;
        case 1: _t->setTranslateLanguages(*reinterpret_cast< QStringList*>(_v)); break;
        case 2: _t->setTranslateLanguage(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setLanguageCode(*reinterpret_cast< QString*>(_v)); break;
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
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 18;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 5;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 5;
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

// SIGNAL 5
void SettingsManager::translateSendIntervalChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
