/****************************************************************************
** Meta object code from reading C++ file 'TranslateManager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../managers/TranslateManager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TranslateManager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TranslateManager_t {
    QByteArrayData data[23];
    char stringdata0[333];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TranslateManager_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TranslateManager_t qt_meta_stringdata_TranslateManager = {
    {
QT_MOC_LITERAL(0, 0, 16), // "TranslateManager"
QT_MOC_LITERAL(1, 17, 12), // "cacheUpdated"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 19), // "isProcessingChanged"
QT_MOC_LITERAL(4, 51, 23), // "translationTaskFinished"
QT_MOC_LITERAL(5, 75, 3), // "pid"
QT_MOC_LITERAL(6, 79, 4), // "flag"
QT_MOC_LITERAL(7, 84, 10), // "extraScope"
QT_MOC_LITERAL(8, 95, 15), // "originalMessage"
QT_MOC_LITERAL(9, 111, 17), // "translatedMessage"
QT_MOC_LITERAL(10, 129, 30), // "requestTranslationWithMetadata"
QT_MOC_LITERAL(11, 160, 7), // "message"
QT_MOC_LITERAL(12, 168, 8), // "language"
QT_MOC_LITERAL(13, 177, 22), // "requestAllTranslations"
QT_MOC_LITERAL(14, 200, 10), // "sourceText"
QT_MOC_LITERAL(15, 211, 14), // "sourceLangCode"
QT_MOC_LITERAL(16, 226, 19), // "translateSingleSync"
QT_MOC_LITERAL(17, 246, 14), // "targetLangCode"
QT_MOC_LITERAL(18, 261, 14), // "getTranslation"
QT_MOC_LITERAL(19, 276, 19), // "getTranslationCount"
QT_MOC_LITERAL(20, 296, 18), // "cancelTasksForText"
QT_MOC_LITERAL(21, 315, 4), // "text"
QT_MOC_LITERAL(22, 320, 12) // "isProcessing"

    },
    "TranslateManager\0cacheUpdated\0\0"
    "isProcessingChanged\0translationTaskFinished\0"
    "pid\0flag\0extraScope\0originalMessage\0"
    "translatedMessage\0requestTranslationWithMetadata\0"
    "message\0language\0requestAllTranslations\0"
    "sourceText\0sourceLangCode\0translateSingleSync\0"
    "targetLangCode\0getTranslation\0"
    "getTranslationCount\0cancelTasksForText\0"
    "text\0isProcessing"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TranslateManager[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       1,  104, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   59,    2, 0x06 /* Public */,
       3,    0,   60,    2, 0x06 /* Public */,
       4,    5,   61,    2, 0x06 /* Public */,

 // methods: name, argc, parameters, tag, flags
      10,    5,   72,    2, 0x02 /* Public */,
      13,    2,   83,    2, 0x02 /* Public */,
      16,    2,   88,    2, 0x02 /* Public */,
      18,    2,   93,    2, 0x02 /* Public */,
      19,    1,   98,    2, 0x02 /* Public */,
      20,    1,  101,    2, 0x02 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt, QMetaType::QString, QMetaType::QString,    5,    6,    7,    8,    9,

 // methods: parameters
    QMetaType::Void, QMetaType::UInt, QMetaType::UInt, QMetaType::UInt, QMetaType::QString, QMetaType::QString,    5,    6,    7,   11,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   14,   15,
    QMetaType::QString, QMetaType::QString, QMetaType::QString,   14,   17,
    QMetaType::QString, QMetaType::QString, QMetaType::QString,   14,   17,
    QMetaType::UChar, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString,   21,

 // properties: name, type, flags
      22, QMetaType::Bool, 0x00495001,

 // properties: notify_signal_id
       1,

       0        // eod
};

void TranslateManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TranslateManager *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->cacheUpdated(); break;
        case 1: _t->isProcessingChanged(); break;
        case 2: _t->translationTaskFinished((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint32(*)>(_a[2])),(*reinterpret_cast< quint32(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< QString(*)>(_a[5]))); break;
        case 3: _t->requestTranslationWithMetadata((*reinterpret_cast< quint32(*)>(_a[1])),(*reinterpret_cast< quint32(*)>(_a[2])),(*reinterpret_cast< quint32(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4])),(*reinterpret_cast< QString(*)>(_a[5]))); break;
        case 4: _t->requestAllTranslations((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 5: { QString _r = _t->translateSingleSync((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 6: { QString _r = _t->getTranslation((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 7: { quint8 _r = _t->getTranslationCount((*reinterpret_cast< const QString(*)>(_a[1])));
            if (_a[0]) *reinterpret_cast< quint8*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->cancelTasksForText((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TranslateManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TranslateManager::cacheUpdated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TranslateManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TranslateManager::isProcessingChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TranslateManager::*)(quint32 , quint32 , quint32 , QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TranslateManager::translationTaskFinished)) {
                *result = 2;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<TranslateManager *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isProcessing(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject TranslateManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_TranslateManager.data,
    qt_meta_data_TranslateManager,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TranslateManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TranslateManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TranslateManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TranslateManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 1;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 1;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void TranslateManager::cacheUpdated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void TranslateManager::isProcessingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void TranslateManager::translationTaskFinished(quint32 _t1, quint32 _t2, quint32 _t3, QString _t4, QString _t5)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
