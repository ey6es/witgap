//
// $Id$

#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QSharedPointer>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpServer>
#include <QVariant>
#include <QVector>

#include "db/PeerRepository.h"
#include "util/Callback.h"
#include "util/General.h"
#include "util/Streaming.h"

class QSslSocket;
class QVariant;

class ArgumentDescriptorList;
class InstanceInfo;
class Peer;
class PeerConnection;
class PendingRequest;
class ServerApp;
class SessionInfo;

typedef QSharedPointer<InstanceInfo> InstanceInfoPointer;
typedef QSharedPointer<SessionInfo> SessionInfoPointer;

typedef QMap<InstanceInfoPointer, int> InstanceInfoPointerMap;

/**
 * Sorts instance info pointers first by open spaces, then by id.
 */
bool operator< (const InstanceInfoPointer& p1, const InstanceInfoPointer& p2);

/**
 * Base class for objects accessible between peers.  This would extend QObject, but there are
 * classes where we prefer to extend from Qt base classes (like QTcpServer).
 */
class SharedObject
{
public:

    /**
     * Sets the shared object identifier.
     */
    void setSharedObjectId (quint32 id) { _sharedObjectId = id; }

    /**
     * Returns the shared object identifier.
     */
    quint32 sharedObjectId () const { return _sharedObjectId; }

protected:

    /** The shared object identifier. */
    quint32 _sharedObjectId;
};

/**
 * Manages peer bits.
 */
class PeerManager : public QTcpServer, public SharedObject
{
    Q_OBJECT

public:

    /**
     * Appends the peer-related command line arguments to the specified list.
     */
    static void appendArguments (ArgumentDescriptorList* args);

    /**
     * Initializes the manager.
     */
    PeerManager (ServerApp* app);

    /**
     * Returns a reference to our own peer record.
     */
    const PeerRecord& record () const { return _record; }

    /**
     * Returns a reference to the secret shared by all peers.
     */
    const QString& sharedSecret () const { return _sharedSecret; }

    /**
     * Configures an SSL socket for peer use.
     */
    void configureSocket (QSslSocket* socket) const;

    /**
     * Registers a shared object (which must be both a QObject and a SharedObject).
     */
    template<class T> void registerSharedObject (T* object);

    /**
     * Retrieves a pointer to the shared object with the specified id, or 0 if not found.
     */
    QObject* sharedObject (int sharedObjectId) const;

    /**
     * Returns a reference to the local session map.
     */
    const QHash<quint64, SessionInfoPointer>& localSessions () const { return _localSessions; }

    /**
     * Returns a reference to the local instance map.
     */
    const QHash<quint64, InstanceInfoPointer>& localInstances () const { return _localInstances; }

    /**
     * Returns the load estimate for the local peer.
     */
    int localLoad () const;

    /**
     * Invokes a method on this peer and all others.  If the invocation includes a callback, it
     * will receive a QVariantListHash mapping peer names to results.  This method is thread-safe.
     *
     * @param object the object on which the invoke the method.
     * @param method the normalized method signature.
     */
    void invoke (SharedObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Invokes a method on all peers other than this one.  If the invocation includes a callback,
     * it will receive a QVariantListHash mapping peer names to results.  This method is
     * thread-safe.
     *
     * @param object the object on which the invoke the method.
     * @param method the normalized method signature.
     */
    void invokeOthers (SharedObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Invokes a method on the lead node (the node whose name comes first in lexicographic order).
     * This method is thread-safe.
     *
     * @param object the object on which the invoke the method.
     * @param method the normalized method signature.
     */
    void invokeLead (SharedObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Invokes a method on the named peer.  If the invocation includes a callback and the peer
     * isn't connected, it will received default-constructed arguments.  This method is
     * thread-safe.
     *
     * @param object the object on which the invoke the method.
     * @param method the normalized method signature.
     */
    void invoke (const QString& peer, SharedObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Invokes a method on peer hosting the named session.  If the invocation includes a callback
     * and the user isn't online, it will received default-constructed arguments.  This method is
     * thread-safe.
     *
     * @param object the object on which the invoke the method, which should be a named child of
     * ServerApp.
     * @param method the normalized method signature.
     */
    void invokeSession (const QString& name, SharedObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Finds an instance id that's not yet in use and supplies it to the given callback.  This is
     * meant to be executed on the lead node.
     *
     * @param peer the name of the peer requesting the id.
     * @param minimumId the minimum id to consider.
     */
    Q_INVOKABLE void reserveInstanceId (
        const QString& peer, quint64 minimumId, const Callback& callback);

    /**
     * Frees up a set of reserved instance ids.
     */
    void freeInstanceIds (const QSet<quint64>& ids) { _reservedInstanceIds.subtract(ids); }

    /**
     * Reserves a place in an instance of the identified zone for the identified session, creating
     * a new instance if necessary.  The callback will receive the peer name and instance id.
     */
    Q_INVOKABLE void reserveInstancePlace (
        quint64 sessionId, quint32 zoneId, const Callback& callback);

    /**
     * Executes an action on this peer and all others.
     */
    template<class T> void execute (const T& action) { execute(QVariant::fromValue(action)); }

    /**
     * Unmaps a destroyed peer.
     */
    void peerDestroyed (Peer* peer);

    /**
     * Notes that a connection has been established by a remote peer.
     */
    void connectionEstablished (PeerConnection* connection);

    /**
     * Notes that a connection has been closed.
     */
    void connectionClosed (PeerConnection* connection);

    /**
     * Called when a session is added on any peer.
     */
    Q_INVOKABLE void sessionAdded (const SessionInfo& info);

    /**
     * Called when a session is updated on any peer.
     */
    Q_INVOKABLE void sessionUpdated (const SessionInfo& info);

    /**
     * Called when a session is removed on any peer.
     */
    Q_INVOKABLE void sessionRemoved (quint64 id, const QString& peer);

    /**
     * Called when an instance is added on any peer.
     */
    Q_INVOKABLE void instanceAdded (const InstanceInfo& info);

    /**
     * Called when an instance is updated on any peer.
     */
    Q_INVOKABLE void instanceUpdated (const InstanceInfo& info);

    /**
     * Called when an instance is removed on any peer.
     */
    Q_INVOKABLE void instanceRemoved (quint64 id);

    /**
     * Executes an action on all peers other than this one.
     */
    Q_INVOKABLE void executeOthers (const QVariant& action);

    /**
     * Makes a request of all peers other than this one.
     */
    Q_INVOKABLE void requestOthers (const QVariant& request, const Callback& callback);

    /**
     * Executes an action on the lead peer.
     */
    Q_INVOKABLE void executeLead (const QVariant& action);

    /**
     * Makes a request of the lead peer.
     */
    Q_INVOKABLE void requestLead (const QVariant& request, const Callback& callback);

    /**
     * Executes an action on the peer hosting the named session.
     */
    Q_INVOKABLE void executeSession (const QString& name, const QVariant& action);

    /**
     * Makes a request of the peer hosting the named session.
     */
    Q_INVOKABLE void requestSession (
        const QString& name, const QVariant& request, const Callback& callback);

protected slots:

    /**
     * Updates our entry in the peer database and (re)loads everyone else's.
     */
    void refreshPeers ();

    /**
     * Deactivates the manager.
     */
    void deactivate ();

protected:

    /**
     * Handles an incoming connection.
     */
    virtual void incomingConnection (int socketDescriptor);

    /**
     * Maps the specified shared object and prepares it for synchronization.
     */
    void mapSharedObject (QObject* object);

    /**
     * Helper function for invocation methods: processes the arguments and returns a variant
     * containing either an InvokeAction or an InvokeRequest.
     */
    QVariant prepareInvoke (SharedObject* object, const char* method,
        QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
        QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
        QGenericArgument val8, QGenericArgument val9, Callback** callback);

    /**
     * Updates our peers based on the list loaded from the database.
     */
    Q_INVOKABLE void updatePeers (const PeerRecordList& records);

    /**
     * Notifies us of the result of an instance creation request.
     */
    Q_INVOKABLE void instanceMaybeCreated (
        quint64 sessionId, quint32 zoneId, const QString& peer,
        const Callback& callback, quint64 instanceId);

    /**
     * Notifies us of the result of an instance place request.
     */
    Q_INVOKABLE void instancePlaceMaybeReserved (
        quint64 sessionId, const QString& peer, quint64 instanceId,
        const Callback& callback, bool success);

    /**
     * Executes an action on this peer and all others.
     */
    Q_INVOKABLE void execute (const QVariant& action);

    /**
     * Makes a request of this peer and all others.
     */
    Q_INVOKABLE void request (const QVariant& request, const Callback& callback);

    /**
     * Executes an action on the named peer.
     */
    Q_INVOKABLE void execute (const QString& name, const QVariant& action);

    /**
     * Makes a request of the named peer.
     */
    Q_INVOKABLE void request (
        const QString& name, const QVariant& request, const Callback& callback);

    /**
     * Handles part of a batch response.
     */
    Q_INVOKABLE void handleResponse (
        quint32 requestId, const QString& name, const QVariantList& args);

    /** The server application. */
    ServerApp* _app;

    /** Our own peer record. */
    PeerRecord _record;

    /** The secret shared by all peers. */
    QString _sharedSecret;

    /** The shared SSL configuration. */
    QSslConfiguration _sslConfig;

    /** The SSL "errors" that we expect (due to the self-signed certificate). */
    QList<QSslError> _expectedSslErrors;

    /** Peers mapped by name. */
    QHash<QString, Peer*> _peers;

    /** The name of the "lead" peer (the one whose name comes first in lexicographic order. */
    QString _leadName;

    /** Incoming connections mapped by peer name. */
    QHash<QString, PeerConnection*> _connections;

    /** Pending requests mapped by id. */
    QHash<quint32, PendingRequest> _pendingRequests;

    /** Shared objects mapped by id. */
    QHash<quint32, QObject*> _sharedObjects;

    /** The last shared object id assigned. */
    quint32 _lastSharedObjectId;

    /** The last request id assigned. */
    quint32 _lastRequestId;

    /** Sessions mapped by id. */
    QHash<quint64, SessionInfoPointer> _sessions;

    /** Sessions mapped by name (converted to lower case). */
    QHash<QString, SessionInfoPointer> _sessionsByName;

    /** Sessions hosted by this peer. */
    QHash<quint64, SessionInfoPointer> _localSessions;

    /** Instances mapped by id. */
    QHash<quint64, InstanceInfoPointer> _instances;

    /** Instances hosted by this peer. */
    QHash<quint64, InstanceInfoPointer> _localInstances;

    /** Instances mapped by zone id. */
    QHash<quint32, InstanceInfoPointerMap> _zoneInstances;

    /** Reserved instance ids. */
    QSet<quint64> _reservedInstanceIds;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
};

template<class T> inline void PeerManager::registerSharedObject (T* object)
{
    object->setSharedObjectId(++_lastSharedObjectId);
    mapSharedObject(object);
}

/**
 * Listens for property changes in order to transmit them to all peers.
 */
class PropertyTransmitter : public QObject
{
    Q_OBJECT

public:

    /**
     * Returns the transmitter for the specified object property, or 0 for none.
     */
    static PropertyTransmitter* instance (QObject* object, const QMetaProperty& property);

    /**
     * Returns the name of the transmitter for the specified property.
     */
    static QByteArray name (const QMetaProperty& property);

    /**
     * Initializes the transmitter.
     */
    PropertyTransmitter (
        ServerApp* app, QObject* object, quint32 sharedObjectId, const QMetaProperty& property);

    /**
     * Destroys the transmitter.
     */
    ~PropertyTransmitter ();

    /**
     * Sets whether or not to ignore property changes.
     */
    void setIgnoreChange (bool ignore) { _ignoreChange = ignore; }

public slots:

    /**
     * Called when the tracked property has changed.
     */
    void propertyChanged ();

protected:

    /** The application object. */
    ServerApp* _app;

    /** The shared object id. */
    quint32 _sharedObjectId;

    /** The property that we watch. */
    QMetaProperty _property;

    /** When set, indicates that we should ignore property changes. */
    bool _ignoreChange;
};

/**
 * Contains information about a session hosted by a peer.
 */
class SessionInfo
{
    STREAMABLE

public:

    /** The session id. */
    STREAM quint64 id;

    /** The session name. */
    STREAM QString name;

    /** The peer name. */
    STREAM QString peer;
};

DECLARE_STREAMABLE_METATYPE(SessionInfo)

/**
 * Contains information about a zone instance hosted by a peer.
 */
class InstanceInfo
{
    STREAMABLE

public:

    /** The instance id. */
    STREAM quint64 id;

    /** The peer name. */
    STREAM QString peer;

    /** The number of spaces currently open in the instance. */
    STREAM quint32 open;
};

DECLARE_STREAMABLE_METATYPE(InstanceInfo)

inline bool operator< (const InstanceInfoPointer& p1, const InstanceInfoPointer& p2)
{
    return (p1->open < p2->open) || (p1->open == p2->open && p1->id < p2->id);
}

/**
 * Contains the state of a pending batch request.
 */
class PendingRequest
{
public:

    /** The callback to invoke with the combined results. */
    Callback callback;

    /** The names of the peers from which we have yet to receive a response. */
    QSet<QString> remaining;

    /** The current set of results. */
    QVariantListHash results;
};

/**
 * Base class for peer actions.
 */
class PeerAction
{
public:

    /**
     * Executes the action.
     */
    virtual void execute (ServerApp* app) const = 0;
};

/**
 * An action that invokes a method.
 */
class InvokeAction : public PeerAction
{
    STREAMABLE

public:

    /** The id of the shared object on which to invoke the action. */
    STREAM quint32 sharedObjectId;

    /** The index of the method to invoke. */
    STREAM quint32 methodIndex;

    /** The arguments with which to invoke the method. */
    STREAM QVariantList args;

    /**
     * Executes the action.
     */
    virtual void execute (ServerApp* app) const;
};

DECLARE_STREAMABLE_METATYPE(InvokeAction)

/**
 * An action that sets a property.
 */
class SetPropertyAction : public PeerAction
{
    STREAMABLE

public:

    /** The id of the shared object on which to set the property. */
    STREAM quint32 sharedObjectId;

    /** The index of the property to set. */
    STREAM quint32 propertyIndex;

    /** The new value. */
    STREAM QVariant value;

    /**
     * Executes the action.
     */
    virtual void execute (ServerApp* app) const;
};

DECLARE_STREAMABLE_METATYPE(SetPropertyAction)

/**
 * Base class for peer requests.
 */
class PeerRequest
{
public:

    /**
     * Handles the request.
     */
    virtual void handle (ServerApp* app, const Callback& callback) const = 0;
};

/**
 * A request that invokes a method.
 */
class InvokeRequest : public PeerRequest
{
    STREAMABLE

public:

    /** The id of the shared object on which to invoke the request. */
    STREAM quint32 sharedObjectId;

    /** The index of the method to invoke. */
    STREAM quint32 methodIndex;

    /** The arguments with which to invoke the method. */
    STREAM QVariantList args;

    /**
     * Handles the request.
     */
    virtual void handle (ServerApp* app, const Callback& callback) const;
};

DECLARE_STREAMABLE_METATYPE(InvokeRequest)

#endif // PEER_MANAGER
