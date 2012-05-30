//
// $Id$

#ifndef CALLBACK
#define CALLBACK

#include <QMetaMethod>
#include <QObject>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QVariant>
#include <QWeakPointer>

/** A guarded pointer to a generic QObject that we can register with the meta-type system. */
typedef QWeakPointer<QObject> QWeakObjectPointer;

class CallablePointer;

/**
 * Contains a weak pointer to a QObject and allows safe dynamic method invocation without concern
 * that the object will be deleted in another thread.
 */
class WeakCallablePointer : public QWeakPointer<QObject>
{
public:

    /**
     * Locks the pointer passed to the constructor as long as the locker remains in scope.
     */
    class Locker
    {
    public:

        /**
         * Initializes the locker, locking the pointer.
         */
        Locker (const WeakCallablePointer* pointer) : _pointer(pointer) { pointer->lock(); }

        /**
         * Destroys the locker, unlocking the pointer.
         */
        ~Locker () { _pointer->unlock(); }

    protected:

        /** The pointer to the object that we're to lock. */
        const WeakCallablePointer* _pointer;
    };

    /**
     * Creates a pointer to the specified object.
     */
    WeakCallablePointer (QObject* object);

    /**
     * Creates a pointer to the specified object.
     */
    WeakCallablePointer (const CallablePointer& pointer);

    /**
     * Creates an invalid weak pointer.
     */
    WeakCallablePointer ();

    /**
     * Attempts to invoke a method on the pointed object.
     *
     * @return true if successful, false if the method wasn't found or the object has been deleted.
     */
    bool invoke (const char* member,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument())
            const;

    /**
     * Attempts to invoke a method on the pointed object.
     *
     * @return true if successful, false if the method wasn't found or the object has been deleted.
     */
    bool invoke (const QMetaMethod& method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument())
            const;

    /**
     * Locks the pointer, ensuring that the pointed object won't be deleted until unlocked.
     */
    void lock () const;

    /**
     * Unlocks the pointer, allowing the reference to be deleted.
     */
    void unlock () const;

    /**
     * Assigns the pointer.
     */
    WeakCallablePointer& operator= (QObject* object);

    /**
     * Assigns the pointer.
     */
    WeakCallablePointer& operator= (const CallablePointer& pointer);

protected:

    /** The lock that coordinates invocation and deletion. */
    QSharedPointer<QReadWriteLock> _lock;
};

/**
 * Represents a callback to an invokable member function of a QObject.  The function will be
 * invoked with the arguments provided to the constructor followed by the arguments
 * provided to the invoke method.  The combined number of arguments must be less than ten.
 */
class Callback
{
public:

    /** The registered metatype id. */
    static const int Type;

    /**
     * Creates a new callback object for the named member.
     *
     * @param member the signature of the member to call.
     */
    Callback (const WeakCallablePointer& object, const char* member,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Creates a new callback for the specified member.
     */
    Callback (const WeakCallablePointer& object, const QMetaMethod& method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Creates an empty, invalid callback object.
     */
    Callback ();

    /**
     * Sets the collate flag, indicating that the arguments passed to {@link #invoke} should be
     * combined into a single QVariantList.
     */
    Callback& setCollate () { _collate = true; return *this; }

    /**
     * Sets the connection type to use when invoking the method.
     */
    Callback& setConnectionType (Qt::ConnectionType type) { _connectionType = type; return *this; }

    /**
     * Invokes the callback.
     */
    void invoke (
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument())
            const;

    /**
     * Invokes the callback with default-constructed arguments.
     */
    void invokeWithDefaults () const;

protected:

    /**
     * Sets the arguments.
     */
    void setArgs (
        QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
        QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
        QGenericArgument val8, QGenericArgument val9);

    /** The object to call. */
    WeakCallablePointer _object;

    /** The method to invoke. */
    QMetaMethod _method;

    /** The constructor-specified callback arguments. */
    QVariantList _args;

    /** If true, combine the arguments passed to the callback into a QVariantList. */
    bool _collate;

    /** The type of connection to use when invoking the method. */
    Qt::ConnectionType _connectionType;
};

/**
 * Wraps a {@link Callback} in a {@link QObject} so that it can be triggered by a signal.
 */
class CallbackObject : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new callback object.
     */
    CallbackObject (const Callback& callback, QObject* parent = 0);

public slots:

    /**
     * Invokes the callback with no additional arguments.
     */
    virtual void invoke () const;

protected:

    /** The wrapped callback. */
    Callback _callback;
};

/**
 * Contains a pointer to a QObject and a lock used to coordinate invocation and deletion.
 */
class CallablePointer : public QSharedPointer<QObject>
{
    friend class WeakCallablePointer;

public:

    /**
     * Initializes the pointer.
     */
    CallablePointer (QObject* object);

    /**
     * Destroys the pointer.
     */
    ~CallablePointer ();

protected:

    /** The lock that prevents simultaneous invocation and deletion. */
    QSharedPointer<QReadWriteLock> _lock;
};

/**
 * Base class for objects that may be deleted while expecting a callback.
 */
class CallableObject : public QObject
{
    Q_OBJECT

    friend class WeakCallablePointer;

public:

    /**
     * Initializes the object.
     */
    CallableObject (QObject* parent = 0);

protected:

    /** The callback pointer. */
    CallablePointer _this;
};

/**
 * Listens for property changes in order to invoke a method with the new value.
 */
class PropertyInvoker : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the invoker.
     */
    PropertyInvoker (
        QObject* object, const QMetaProperty& property,
        const WeakCallablePointer& target, const char* method);

    /**
     * Initializes the invoker.
     */
    PropertyInvoker (
        QObject* object, const QMetaProperty& property,
        const WeakCallablePointer& target, const QMetaMethod& method);

    /**
     * Sets the property.
     */
    Q_INVOKABLE void setProperty (const QVariant& value);

public slots:

    /**
     * Called when the tracked property has changed.
     */
    void propertyChanged ();

protected:

    /** The property that we watch. */
    QMetaProperty _property;

    /** The object to call. */
    WeakCallablePointer _target;

    /** The method that we invoke with new values. */
    QMetaMethod _method;
};

class PropertySynchronizer : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Initializes the synchronizer.
     */
    PropertySynchronizer (QObject* local, QObject* remote);

    /**
     * Applies the property values in the local object to the remote one.
     */
    void apply ();

    /**
     * Sets the specified property.
     */
    Q_INVOKABLE void setProperty (const QMetaProperty& property, const QVariant& value);

protected:

    /** The object to synchronize with. */
    QObject* _remote;
};

#endif // CALLBACK
