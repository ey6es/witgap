//
// $Id$

#include <limits>

#include <QtDebug>

#include "util/Callback.h"

using namespace std;

// register our types with the metatype system
const int Callback::Type = qRegisterMetaType<Callback>("Callback");
int qWeakObjectPointerType = qRegisterMetaType<QWeakObjectPointer>("QWeakObjectPointer");

Callback::Callback (QObject* object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _method(object->metaObject()->method(object->metaObject()->indexOfMethod(method))),
        _collate(false)
{
    setObject(object);
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback (QObject* object, QMetaMethod method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _method(method),
        _collate(false)
{
    setObject(object);
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback (const CallablePointer& pointer, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _method(pointer->metaObject()->method(pointer->metaObject()->indexOfMethod(method))),
        _object(pointer),
        _semaphore(pointer._semaphore),
        _collate(false)
{
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback (const CallablePointer& pointer, QMetaMethod method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) :
        _method(method),
        _object(pointer),
        _semaphore(pointer._semaphore),
        _collate(false)
{
    setArgs(val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

Callback::Callback ()
{
}

void Callback::setObject (QObject* object)
{
    CallableObject* cobj = qobject_cast<CallableObject*>(object);
    if (cobj == 0) {
        _object = object;
    } else {
        _object = cobj->_this;
        _semaphore = cobj->_this._semaphore;
    }
}

void Callback::setArgs (
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    QGenericArgument args[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
    for (int ii = 0; ii < 10 && args[ii].name() != 0; ii++) {
        _args[ii] = QVariant(QMetaType::type(args[ii].name()), args[ii].data());
    }
}

void Callback::invoke (
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9) const
{
    // acquire a resource if we were supplied a semaphore
    if (_semaphore) {
        _semaphore->acquire();
    }

    // invoke the method if the object reference is still valid
    QObject* data = _object.data();
    if (data != 0) {
        QGenericArgument nargs[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
        QGenericArgument cargs[10];

        // combine the callback arguments with the invocation arguments
        int idx = 0;
        for (; idx < 10 && _args[idx].isValid(); idx++) {
            cargs[idx] = QGenericArgument(_args[idx].typeName(), _args[idx].constData());
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
        _method.invoke(data, cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
            cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
    }

    // release the resource if acquired
    if (_semaphore) {
        _semaphore->release();
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
    _semaphore(new QSemaphore(numeric_limits<int>::max()))
{
}

CallablePointer::~CallablePointer ()
{
    // acquire all the resources
    _semaphore->acquire(numeric_limits<int>::max());

    // note that the object is no longer available
    clear();

    // release all the resources
    _semaphore->release(numeric_limits<int>::max());
}

CallableObject::CallableObject (QObject* parent) :
    QObject(parent),
    _this(this)
{
}
