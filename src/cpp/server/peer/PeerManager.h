//
// $Id$

#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <QHash>
#include <QTcpServer>

#include "db/PeerRepository.h"
#include "util/Callback.h"

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
     * Updates our peers based on the list loaded from the database.
     */
    Q_INVOKABLE void updatePeers (const PeerRecordList& records);

    /** The server application. */
    ServerApp* _app;

    /** Our own peer record. */
    PeerRecord _record;

    /** Peers mapped by name. */
    QHash<QString, Peer*> _peers;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
};

#endif // PEER_MANAGER
