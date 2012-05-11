//
// $Id$

#ifndef CALLBACK
#define CALLBACK

#include <QMetaMethod>
#include <QObject>
#include <QSemaphore>
#include <QSharedPointer>
#include <QVariant>
#include <QWeakPointer>

/** A guarded pointer to a generic QObject that we can register with the meta-type system. */
typedef QWeakPointer<QObject> QWeakObjectPointer;

class CallablePointer;

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
    Callback (QObject* object, const char* member,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Creates a new callback for the specified member.
     */
    Callback (QObject* object, QMetaMethod method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Creates a new callback object for the named member.
     *
     * @param member the signature of the member to call.
     */
    Callback (const CallablePointer& pointer, const char* member,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Creates a new callback for the specified member.
     */
    Callback (const CallablePointer& pointer, QMetaMethod method,
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
     * Invokes the callback.
     */
    void invoke (
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument())
            const;

protected:

    /**
     * Sets the object pointer and associated state.
     */
    void setObject (QObject* object);

    /**
     * Sets the arguments.
     */
    void setArgs (
        QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
        QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
        QGenericArgument val8, QGenericArgument val9);

    /** The object to call. */
    QWeakPointer<QObject> _object;

    /** The mutex that coordinates invocation and deletion. */
    QSharedPointer<QSemaphore> _semaphore;

    /** The method to invoke. */
    QMetaMethod _method;

    /** The constructor-specified callback arguments. */
    QVariant _args[10];

    /** If true, combine the arguments passed to the callback into a QVariantList. */
    bool _collate;
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
 * Contains a pointer to a QObject and a semaphore used to coordinate invocation and deletion.
 */
class CallablePointer : public QSharedPointer<QObject>
{
    friend class Callback;

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

    /** The semaphore that prevents simultaneous invocation and deletion. */
    QSharedPointer<QSemaphore> _semaphore;
};

/**
 * Base class for objects that may be deleted while expecting a callback.
 */
class CallableObject : public QObject
{
    Q_OBJECT

    friend class Callback;

public:

    /**
     * Initializes the object.
     */
    CallableObject (QObject* parent = 0);

protected:

    /** The callback pointer. */
    CallablePointer _this;
};

#endif // CALLBACK
