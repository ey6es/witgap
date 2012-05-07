//
// $Id$

#ifndef PEER_CONNECTION
#define PEER_CONNECTION

#include <QHostAddress>
#include <QObject>

class QSslSocket;

class ServerApp;

/**
 * Handles a single incoming peer connection.
 */
class PeerConnection : public QObject
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
    ~PeerConnection ();

protected:

    /** The server application. */
    ServerApp* _app;

    /** The underlying socket. */
    QSslSocket* _socket;

    /** The stored address. */
    QHostAddress _address;
};

#endif // PEER_CONNECTION
