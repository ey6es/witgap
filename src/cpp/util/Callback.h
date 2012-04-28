//
// $Id$

#ifndef CALLBACK
#define CALLBACK

#include <QMetaMethod>
#include <QObject>
#include <QPair>
#include <QSharedPointer>
#include <QWeakPointer>

/** A guarded pointer to a generic QObject that we can register with the meta-type system. */
typedef QWeakPointer<QObject> QWeakObjectPointer;

class CallablePointer;
class QSemaphore;

/**
 * Represents a callback to an invokable member function of a QObject.  The function will be
 * invoked with the arguments provided to the constructor followed by the arguments
 * provided to the invoke method.  The combined number of arguments must be less than ten.
 */
class Callback
{
public:

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
     * Copy constructor.
     */
    Callback (const Callback& other);

    /**
     * Creates an empty, invalid callback object.
     */
    Callback ();

    /**
     * Destroys the callback.
     */
    ~Callback ();

    /**
     * Assignment operator.
     */
    Callback& operator= (const Callback& other);

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
     * Copies the provided arguments.
     */
    void copyArguments (const QGenericArgument args[10]);

    /**
     * Copies the provided arguments.
     */
    void copyArguments (const QPair<int, void*> args[10]);

    /** The object to call. */
    QWeakPointer<QObject> _object;

    /** The mutex that coordinates invocation and deletion. */
    QSharedPointer<QSemaphore> _semaphore;

    /** The method to invoke. */
    QMetaMethod _method;

    /** The constructor-specified callback arguments. */
    QPair<int, void*> _args[10];
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

/**
 * Provides a means to create an object on receipt of a signal.
 */
class Creator : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new creator.
     */
    Creator (QObject* parent, const QMetaObject* metaObject,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Destroys the creator.
     */
    virtual ~Creator ();

public slots:

    /**
     * Creates the configured object.
     */
    QObject* create ();

protected:

    /** The meta-object to instantiate. */
    const QMetaObject* _metaObject;

    /** The constructor-specified constructor arguments. */
    QPair<int, void*> _args[10];
};

#endif // CALLBACK
