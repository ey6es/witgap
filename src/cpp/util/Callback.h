//
// $Id$

#ifndef CALLBACK
#define CALLBACK

#include <QCoreApplication>
#include <QMetaMethod>
#include <QObject>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QThreadStorage>
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
 * Provides a means of synchronizing property values between objects of the same class living in
 * different threads.
 */
class ObjectSynchronizer : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the synchronizer.
     *
     * @param autoApply if true, automatically apply changes made to the copy to the original.
     */
    ObjectSynchronizer (QObject* original, QObject* copy, bool autoApply = false);

    /**
     * Applies the changes made in the copy to the original.
     */
    void apply () { emit applied(); }

signals:

    /**
     * Fired when the apply method is called.
     */
    void applied ();
};

/**
 * Handles the synchronization of a single property.
 */
class PropertySynchronizer : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Initializes the synchronizer.
     */
    PropertySynchronizer (QObject* object, const QMetaProperty& property);

    /**
     * Sets our counterpart pointer.
     */
    void setCounterpart (const WeakCallablePointer& counterpart);

    /**
     * Initializes the synchronizer with its parent.
     */
    Q_INVOKABLE void init (QObject* parent);

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

    /** Our counterpart on the other object. */
    WeakCallablePointer _counterpart;
};

/**
 * Stores a pointer to an original object, creating synchronized copies as necessary for threads
 * other than the object's.
 */
template<class T> class SynchronizedStorage : protected QThreadStorage<T*>
{
public:

    /**
     * Creates synchronized storage for the provided original.
     */
    SynchronizedStorage (T* original) : _original(original) { setLocalData(original); }

    /**
     * Returns the original or a local copy, depending on the parameter.
     */
    T* get (bool original) { return original ? _original : local(); }

    /**
     * Returns a pointer to the original.
     */
    T* original () const { return _original; }

    /**
     * Returns a pointer to the original or a synchronized copy thereof, as appropriate for the
     * calling thread.
     */
    T* local ();

    /**
     * Prefetches the copy for the specified thread.
     */
    void prefetch (QThread* thread);

protected:

    /** A pointer to the original object. */
    T* _original;
};

template<class T> inline T* SynchronizedStorage<T>::local ()
{
    T* data = QThreadStorage<T*>::localData();
    if (data == 0) {
        setLocalData(data = new T());
        new ObjectSynchronizer(_original, data, true);
    }
    return data;
}

/**
 * Supports prefetching a synchronized storage object for a specific thread.
 */
template<class T> class SynchronizedStoragePrefetcher : public QObject
{
public:
    
    /**
     * Creates a new prefetcher.
     */
    SynchronizedStoragePrefetcher (SynchronizedStorage<T>* storage) : _storage(storage) { }

    /**
     * Handles a custom event.
     */
    virtual void customEvent (QEvent* event) { _storage->local(); deleteLater(); }
    
protected:
    
    /** The storage that we prefetch. */
    SynchronizedStorage<T>* _storage;
};

template<class T> inline void SynchronizedStorage<T>::prefetch (QThread* thread)
{
    SynchronizedStoragePrefetcher<T>* prefetcher = new SynchronizedStoragePrefetcher<T>(this);
    prefetcher->moveToThread(thread);
    QCoreApplication::postEvent(prefetcher, new QEvent(QEvent::User));
}

#endif // CALLBACK
