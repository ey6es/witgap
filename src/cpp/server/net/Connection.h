//
// $Id$

#ifndef CONNECTION
#define CONNECTION

#include <QMetaMethod>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTcpSocket>

#include "util/General.h"

class ServerApp;

/**
 * Handles a single TCP connection.
 */
class Connection : public QObject
{
    Q_OBJECT

public:

    /**
     * Returns the meta-method for {@link #addWindow}.
     */
    static const QMetaMethod& addWindowMetaMethod ();

    /**
     * Returns the meta-method for {@link #removeWindow}.
     */
    static const QMetaMethod& removeWindowMetaMethod ();

    /**
     * Returns the meta-method for {@link #updateWindow}.
     */
    static const QMetaMethod& updateWindowMetaMethod ();

    /**
     * Returns the meta-method for {@link #setContents}.
     */
    static const QMetaMethod& setContentsMetaMethod ();

    /**
     * Initializes the connection.
     */
    Connection (ServerApp* app, QTcpSocket* socket);

    /**
     * Checks whether the connection is open.
     */
    bool isOpen () const { return _socket->state() == QAbstractSocket::ConnectedState; };

    /**
     * Returns the display size reported by the client.
     */
    const QSize& displaySize () const { return _displaySize; }

    /**
     * Activates the connection, allowing it to begin reading and writing messages.
     */
    void activate ();

    /**
     * Deactivates the connection, forcibly closing and deleting it.
     */
    void deactivate (const QString& reason);

    /**
     * Adds a window to the user's display.
     */
    Q_INVOKABLE void addWindow (int id, int layer, const QRect& bounds, int fill);

    /**
     * Removes a window from the user's display.
     */
    Q_INVOKABLE void removeWindow (int id);

    /**
     * Updates a window on the user's display.
     */
    Q_INVOKABLE void updateWindow (int id, int layer, const QRect& bounds, int fill);

    /**
     * Sets part of a window's contents.
     */
    Q_INVOKABLE void setContents (int id, const QRect& bounds, const QIntVector& contents);

    /**
     * Moves part of a window's contents.
     */
    Q_INVOKABLE void moveContents (int id, const QRect& source, const QPoint& dest, int fill);

    /**
     * Sets the user's session id/token.
     */
    Q_INVOKABLE void setSession (quint64 id, const QByteArray& token);

signals:

    /**
     * Fired when the user presses the mouse.
     */
    void mousePressed (int x, int y);

    /**
     * Fired when the user releases the mouse.
     */
    void mouseReleased (int x, int y);

    /**
     * Fired when the user presses a key.
     */
    void keyPressed (int key);

    /**
     * Fired when the user releases a key.
     */
    void keyReleased (int key);

    /**
     * Fired when the user closes the window.
     */
    void windowClosed ();

protected slots:

    /**
     * Reads a chunk of incoming header (before transitioning to messages).
     */
    void readHeader ();

    /**
     * Reads a chunk of incoming messages (after validating header).
     */
    void readMessages ();

protected:

    /**
     * Writes a rectangle to the stream.
     */
    void write (const QRect& rect);

    /** The server application. */
    ServerApp* _app;

    /** The underlying socket. */
    QTcpSocket* _socket;

    /** The data stream used to read from and write to the socket. */
    QDataStream _stream;

    /** The display size reported by the client. */
    QSize _displaySize;
};

#endif // CONNECTION
