//
// $Id$

#include <limits>

#include <QByteArray>
#include <QList>
#include <QtDebug>

#include "util/Callback.h"
#include "util/General.h"

using namespace std;

// register our types with the metatype system
const int Callback::Type = qRegisterMetaType<Callback>("Callback");
int qWeakObjectPointerType = qRegisterMetaType<QWeakObjectPointer>("QWeakObjectPointer");

WeakCallablePointer::WeakCallablePointer (QObject* object)
{
    *this = object;
}

WeakCallablePointer::WeakCallablePointer (const CallablePointer& pointer) :
    QWeakPointer<QObject>(pointer),
    _lock(pointer._lock)
{
}

WeakCallablePointer::WeakCallablePointer ()
{
}

bool WeakCallablePointer::invoke (const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) const
{
    Locker locker(this);
    QObject* object = data();
    return object != 0 && QMetaObject::invokeMethod(object, method, val0, val1, val2,
        val3, val4, val5, val6, val7, val8, val9);
}

bool WeakCallablePointer::invoke (const QMetaMethod& method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) const
{
    Locker locker(this);
    QObject* object = data();
    return object != 0 && method.invoke(object, val0, val1, val2, val3, val4,
        val5, val6, val7, val8, val9);
}

void WeakCallablePointer::lock () const
{
    if (_lock) {
        _lock->lockForRead();
    }
}

void WeakCallablePointer::unlock () const
{
    if (_lock) {
        _lock->unlock();
    }
}

WeakCallablePointer& WeakCallablePointer::operator= (QObject* object)
{
    CallableObject* cobj = qobject_cast<CallableObject*>(object);
    if (cobj == 0) {
        *((QWeakPointer<QObject>*)this) = object;
    } else {
        *this = cobj->_this;
    }
}

WeakCallablePointer& WeakCallablePointer::operator= (const CallablePointer& pointer)
{
    *((QWeakPointer<QObject>*)this) = pointer;
    _lock = pointer._lock;
}

/**
 * Helper function for method resolution: returns the meta-method with the supplied
 * signature.
 */
static QMetaMethod metaMethod (const QMetaObject* metaObject, const char* signature)
{
    return metaObject->method(metaObject->indexOfMethod(signature));
}

Callback::Callback (const WeakCallablePointer& object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _object(object),
        _method(metaMethod(object.data()->metaObject(), method)),
        _collate(false),
        _connectionType(Qt::AutoConnection)
{
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback (const WeakCallablePointer& object, const QMetaMethod& method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _object(object),
        _method(method),
        _collate(false),
        _connectionType(Qt::AutoConnection)
{
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback ()
{
}

void Callback::setArgs (
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    QGenericArgument args[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
    for (int ii = 0; ii < 10 && args[ii].name() != 0; ii++) {
        _args.append(QVariant(QMetaType::type(args[ii].name()), args[ii].data()));
    }
}

void Callback::invoke (
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) const
{
    // lock our pointer to prevent it from being deleted
    WeakCallablePointer::Locker locker(&_object);

    // invoke the method if the object reference is still valid
    QObject* data = _object.data();
    if (data != 0) {
        QGenericArgument nargs[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
        QGenericArgument cargs[10];

        // combine the callback arguments with the invocation arguments
        int idx = 0;
        for (int nn = _args.size(); idx < nn; idx++) {
            const QVariant& arg = _args.at(idx);
            cargs[idx] = QGenericArgument(arg.typeName(), arg.constData());
        }
        QVariantList list;
        if (_collate) {
            for (int ii = 0; ii < 10 && nargs[ii].name() != 0; ii++) {
                list.append(QVariant(QMetaType::type(nargs[ii].name()), nargs[ii].data()));
            }
            cargs[idx] = Q_ARG(const QVariantList&, list);

        } else {
            for (int ii = 0; idx < 10 && nargs[ii].name() != 0; ii++, idx++) {
                cargs[idx] = nargs[ii];
            }
        }
        _method.invoke(data, _connectionType, cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
            cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
    }
}

void Callback::invokeWithDefaults () const
{
    int types[10];
    QGenericArgument nargs[10];
    QList<QByteArray> ptypes = _method.parameterTypes();
    for (int ii = 0, idx = _args.size(), nn = ptypes.size(); idx < nn; ii++, idx++) {
        const char* typeName = ptypes.at(idx).constData();
        types[ii] = QMetaType::type(typeName);
        nargs[ii] = QGenericArgument(typeName, QMetaType::construct(types[ii]));
    }
    invoke(nargs[0], nargs[1], nargs[2], nargs[3], nargs[4],
        nargs[5], nargs[6], nargs[7], nargs[8], nargs[9]);
    for (int ii = 0; ii < 10 && nargs[ii].name() != 0; ii++) {
        QMetaType::destroy(types[ii], nargs[ii].data());
    }
}

CallbackObject::CallbackObject (const Callback& callback, QObject* parent) :
    QObject(parent),
    _callback(callback)
{
}

void CallbackObject::invoke () const
{
    _callback.invoke();
}

/**
 * A "Deleter" that does nothing (it will only be called from the destructor).
 */
static void noopDeleter (QObject* obj)
{
}

CallablePointer::CallablePointer (QObject* object) :
    QSharedPointer<QObject>(object, noopDeleter),
    _lock(new QReadWriteLock())
{
}

CallablePointer::~CallablePointer ()
{
    // note that the object is no longer available
    _lock->lockForWrite();
    clear();
    _lock->unlock();
}

CallableObject::CallableObject (QObject* parent) :
    QObject(parent),
    _this(this)
{
}

ObjectSynchronizer::ObjectSynchronizer (QObject* original, QObject* copy, bool autoApply) :
    QObject(copy)
{
    const QMetaObject* metaObject = copy->metaObject();
    for (int ii = 0, nn = metaObject->propertyCount(); ii < nn; ii++) {
        QMetaProperty property = metaObject->property(ii);
        if (property.hasNotifySignal()) {
            PropertySynchronizer* osync = new PropertySynchronizer(0, property);
            PropertySynchronizer* csync = new PropertySynchronizer(copy, property);
            osync->setCounterpart(csync);
            csync->setCounterpart(osync);

            // move to appropriate thread and initialize there
            osync->moveToThread(original->thread());
            QMetaObject::invokeMethod(osync, "init", Q_ARG(QObject*, original));

            // transfer to original when the copy changes/is applied
            if (autoApply) {
                csync->connect(copy, signal(property.notifySignal().signature()),
                    SLOT(propertyChanged()));
            } else {
                csync->connect(this, SIGNAL(applied()), SLOT(propertyChanged()));
            }
        }
    }
}

PropertySynchronizer::PropertySynchronizer (QObject* object, const QMetaProperty& property) :
    CallableObject(object),
    _property(property)
{
}

void PropertySynchronizer::setCounterpart (const WeakCallablePointer& counterpart)
{
    _counterpart = counterpart;
    connect(counterpart.data(), SIGNAL(destroyed()), SLOT(deleteLater()));
}

void PropertySynchronizer::init (QObject* parent)
{
    setParent(parent);
    connect(parent, signal(_property.notifySignal().signature()), SLOT(propertyChanged()));
    propertyChanged();
}

void PropertySynchronizer::setProperty (const QVariant& value)
{
    _property.write(parent(), value);
}

void PropertySynchronizer::propertyChanged ()
{
    _counterpart.invoke("setProperty", Q_ARG(const QVariant&, _property.read(parent())));
}
