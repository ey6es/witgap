//
// $Id$

#ifndef CONNECTION
#define CONNECTION

#include <QPoint>
#include <QRect>
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

signals:

    /**
     * Fired when the user presses a key.
     */
    void keyPressed (int key);

    /**
     * Fired when the user releases a key.
     */
    void keyReleased (int key);

public slots:

    /**
     * Adds a window to the user's display.
     */
    void addWindow (int id, int layer, const QRect& bounds, int fill);

    /**
     * Removes a window from the user's display.
     */
    void removeWindow (int id);

    /**
     * Updates a window on the user's display.
     */
    void updateWindow (int id, int layer, const QRect& bounds, int fill);

    /**
     * Sets part of a window's contents.
     */
    void setContents (int id, const QRect& bounds);

    /**
     * Moves part of a window's contents.
     */
    void moveContents (int id, const QRect& source, const QPoint& dest, int fill);

    /**
     * Sets the user's session token.
     */
    void setSession ();

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
