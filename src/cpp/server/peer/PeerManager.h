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

#include "db/PeerRepository.h"
#include "util/Callback.h"

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
     * Executes an action on this peer and all others.
     */
    void execute (const QVariant& action);

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

#endif // PEER_MANAGER
