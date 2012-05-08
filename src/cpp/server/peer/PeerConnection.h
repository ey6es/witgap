//
// $Id$

#ifndef PEER_CONNECTION
#define PEER_CONNECTION

#include <QHostAddress>

#include "peer/AbstractPeer.h"

/**
 * Handles a single incoming peer connection.
 */
class PeerConnection : public AbstractPeer
{
    Q_OBJECT

public:

    /**
     * Initializes the connection.
     */
    PeerConnection (ServerApp* app, QSslSocket* socket);

    /**
     * Destroys the connection.
     */
    virtual ~PeerConnection ();

protected slots:

    /**
     * Reads a chunk of incoming header (before transitioning to messages).
     */
    void readHeader ();

protected:

    /**
     * Handles an incoming message.
     */
    virtual void handle (const PeerMessage* message);

    /** The stored address. */
    QHostAddress _address;
};

#endif // PEER_CONNECTION
