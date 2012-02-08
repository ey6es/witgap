//
// $Id$

#ifndef CONNECTION
#define CONNECTION

#include <QTcpSocket>

class ServerApp;

/**
 * Handles a single TCP connection.
 */
class Connection : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the connection.
     */
    Connection (ServerApp* app, QTcpSocket* socket);

    /**
     * Destroys the connection.
     */
    ~Connection ();

    /**
     * Returns a reference to the underlying socket.
     */
    QTcpSocket* socket () const { return _socket; };

protected slots:

    /**
     * Reads a chunk of incoming header (before transitioning to messages).
     */
    void readHeader ();

    /**
     * Reads a chunk of incoming messages (after validating header).
     */
    void readMessages ();

    /**
     * Handles an error on the socket.
     */
    void handleError (QAbstractSocket::SocketError error);

protected:

    /** The server application. */
    ServerApp* _app;

    /** The underlying socket. */
    QTcpSocket* _socket;
};

#endif // CONNECTION
