//
// $Id$

#ifndef PEER
#define PEER

#include <QObject>

#include "db/PeerRepository.h"

class QSslSocket;

class ServerApp;

/**
 * Handles a single peer.
 */
class Peer : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the peer.
     */
    Peer (ServerApp* app);

    /**
     * Updates the peer with its most recent record.
     */
    void update (const PeerRecord& record);

protected slots:

    /**
     * Attempts to reconnect, since we lost the connection.
     */
    void reconnectLater ();

    /**
     * Attempts to connect to the peer.
     */
    void connectToPeer ();

protected:

    /**
     * Returns the hostname to use when connecting to the peer.
     */
    const QString& hostname () const;

    /** The server application. */
    ServerApp* _app;

    /** The latest peer record. */
    PeerRecord _record;

    /** The socket through which we connect. */
    QSslSocket* _socket;
};

#endif // PEER
