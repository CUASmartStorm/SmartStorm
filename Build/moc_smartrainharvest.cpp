/****************************************************************************
** Meta object code from reading C++ file 'smartrainharvest.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.8)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../smartrainharvest.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'smartrainharvest.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.8. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SmartRainHarvest_t {
    QByteArrayData data[8];
    char stringdata0[116];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SmartRainHarvest_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SmartRainHarvest_t qt_meta_stringdata_SmartRainHarvest = {
    {
QT_MOC_LITERAL(0, 0, 16), // "SmartRainHarvest"
QT_MOC_LITERAL(1, 17, 16), // "onMonitoringTick"
QT_MOC_LITERAL(2, 34, 0), // ""
QT_MOC_LITERAL(3, 35, 13), // "onReleaseTick"
QT_MOC_LITERAL(4, 49, 16), // "onManualOpenShut"
QT_MOC_LITERAL(5, 66, 20), // "onAutoControlToggled"
QT_MOC_LITERAL(6, 87, 7), // "checked"
QT_MOC_LITERAL(7, 95, 20) // "checkIfShouldRelease"

    },
    "SmartRainHarvest\0onMonitoringTick\0\0"
    "onReleaseTick\0onManualOpenShut\0"
    "onAutoControlToggled\0checked\0"
    "checkIfShouldRelease"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SmartRainHarvest[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   39,    2, 0x08 /* Private */,
       3,    0,   40,    2, 0x08 /* Private */,
       4,    0,   41,    2, 0x08 /* Private */,
       5,    1,   42,    2, 0x08 /* Private */,
       7,    0,   45,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    6,
    QMetaType::Bool,

       0        // eod
};

void SmartRainHarvest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SmartRainHarvest *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onMonitoringTick(); break;
        case 1: _t->onReleaseTick(); break;
        case 2: _t->onManualOpenShut(); break;
        case 3: _t->onAutoControlToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: { bool _r = _t->checkIfShouldRelease();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SmartRainHarvest::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_SmartRainHarvest.data,
    qt_meta_data_SmartRainHarvest,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SmartRainHarvest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SmartRainHarvest::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SmartRainHarvest.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int SmartRainHarvest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
