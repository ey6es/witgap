//
// $Id$

#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpServer>
#include <QVariant>

#include "db/PeerRepository.h"
#include "util/Callback.h"
#include "util/Streaming.h"

class QSslSocket;
class QVariant;

class ArgumentDescriptorList;
class Peer;
class ServerApp;

/**
 * Manages peer bits.
 */
class PeerManager : public QTcpServer
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
    const QByteArray& sharedSecret () const { return _sharedSecret; }

    /**
     * Configures an SSL socket for peer use.
     */
    void configureSocket (QSslSocket* socket) const;

    /**
     * Invokes a method on this peer and all others.  This method is thread-safe.
     *
     * @param object the object on which the invoke the method, which should be a named child of
     * ServerApp.
     * @param method the normalized method signature.
     */
    void invoke (QObject* object, const char* method,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Executes an action on this peer and all others.
     */
    template<class T> void execute (const T& action) { execute(QVariant::fromValue(action)); }

protected slots:

    /**
     * Updates our entry in the peer database and (re)loads everyone else's.
     */
    void refreshPeers ();

    /**
     * Unmaps a destroyed peer.
     */
    void unmapPeer (QObject* object);

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
     * Updates our peers based on the list loaded from the database.
     */
    Q_INVOKABLE void updatePeers (const PeerRecordList& records);

    /**
     * Executes an action on this peer and all others.
     */
    Q_INVOKABLE void execute (const QVariant& action);

    /** The server application. */
    ServerApp* _app;

    /** Our own peer record. */
    PeerRecord _record;

    /** The secret shared by all peers. */
    QByteArray _sharedSecret;

    /** The shared SSL configuration. */
    QSslConfiguration _sslConfig;

    /** The SSL "errors" that we expect (due to the self-signed certificate). */
    QList<QSslError> _expectedSslErrors;

    /** Peers mapped by name. */
    QHash<QString, Peer*> _peers;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
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
class InvokeAction
{
    STREAMABLE

public:

    /** The name of the object on which to invoke the action. */
    STREAM QString name;

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

#endif // PEER_MANAGER
