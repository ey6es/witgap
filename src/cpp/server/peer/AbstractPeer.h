//
// $Id$

#ifndef ABSTRACT_PEER
#define ABSTRACT_PEER

#include <QDataStream>
#include <QObject>

class QSslSocket;

class PeerMessage;
class ServerApp;

/**
 * Base class for {@link Peer} and {@link PeerConnection}.
 */
class AbstractPeer : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the peer.
     */
    AbstractPeer (ServerApp* app, QSslSocket* socket);

    /**
     * Destroys the peer.
     */
    virtual ~AbstractPeer ();

    /**
     * Sends a message to the connected peer.
     */
    void sendMessage (const PeerMessage* message);

protected slots:

    /**
     * Reads any incoming messages.
     */
    void readMessages ();

protected:

    /**
     * Handles an incoming message.
     */
    virtual void handle (const PeerMessage* message) = 0;

    /** The server application. */
    ServerApp* _app;

    /** The socket through which we connect. */
    QSslSocket* _socket;

    /** The data stream for streaming. */
    QDataStream _stream;
};

#endif // ABSTRACT_PEER
