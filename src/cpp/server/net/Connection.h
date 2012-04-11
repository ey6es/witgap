//
// $Id$

#ifndef CONNECTION
#define CONNECTION

#include <QHash>
#include <QHostAddress>
#include <QMultiHash>
#include <QSize>
#include <QTcpSocket>

#include <openssl/evp.h>

#include "util/Callback.h"

class QMetaMethod;
class QIntVector;
class QPoint;
class QRect;

class ServerApp;

/**
 * Handles a single TCP connection.
 */
class Connection : public QObject
{
    Q_OBJECT

public:

    /**
     * Returns the meta-method for {@link #updateWindow}.
     */
    static const QMetaMethod& updateWindowMetaMethod ();

    /**
     * Returns the meta-method for {@link #removeWindow}.
     */
    static const QMetaMethod& removeWindowMetaMethod ();

    /**
     * Returns the meta-method for {@link #setContents}.
     */
    static const QMetaMethod& setContentsMetaMethod ();

    /**
     * Returns the meta-method for {@link #moveContents}.
     */
    static const QMetaMethod& moveContentsMetaMethod ();

    /**
     * Returns the meta-method for {@link #setCookie}.
     */
    static const QMetaMethod& setCookieMetaMethod ();

    /**
     * Returns the meta-method for {@link #toggleCrypto}.
     */
    static const QMetaMethod& toggleCryptoMetaMethod ();

    /**
     * Initializes the connection.
     */
    Connection (ServerApp* app, QTcpSocket* socket);

    /**
     * Destroys the connection.
     */
    ~Connection ();

    /**
     * Checks whether the connection is open.
     */
    bool isOpen () const { return _socket->state() == QAbstractSocket::ConnectedState; };

    /**
     * Returns the display size reported by the client.
     */
    const QSize& displaySize () const { return _displaySize; }

    /**
     * Returns a reference to the map of query parameters passed to the client.
     */
    const QMultiHash<QString, QString> query () const { return _query; }

    /**
     * Returns a reference to the map of cookie values stored on the client.
     */
    const QHash<QString, QString> cookies () const { return _cookies; }

    /**
     * Activates the connection, allowing it to begin reading and writing messages.
     */
    void activate ();

    /**
     * Deactivates the connection, forcibly closing and deleting it.
     */
    void deactivate (const QString& reason);

    /**
     * Updates a window on the user's display.
     */
    Q_INVOKABLE void updateWindow (int id, int layer, const QRect& bounds, int fill);

    /**
     * Removes a window from the user's display.
     */
    Q_INVOKABLE void removeWindow (int id);

    /**
     * Sets part of a window's contents.
     */
    Q_INVOKABLE void setContents (int id, const QRect& bounds, const QIntVector& contents);

    /**
     * Moves part of a window's contents.
     */
    Q_INVOKABLE void moveContents (int id, const QRect& source, const QPoint& dest, int fill);

    /**
     * Sets a client cookie.
     */
    Q_INVOKABLE void setCookie (const QString& name, const QString& value);

    /**
     * Toggles encryption.
     */
    Q_INVOKABLE void toggleCrypto ();

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
    void keyPressed (int key, QChar ch, bool numpad);

    /**
     * Fired when the user releases a key.
     */
    void keyReleased (int key, QChar ch, bool numpad);

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
     * Starts a message of the specified (unencrypted) length.
     */
    void startMessage (quint16 length);

    /**
     * Finishes the current message.
     */
    void endMessage ();

    /**
     * Writes a rectangle to the stream.
     */
    void write (const QRect& rect);

    /**
     * Reads an encrypted block of the specified length and returns the decrypted result.
     */
    QByteArray readEncrypted (int length);

    /**
     * Encrypts the provided block and writes it out.
     */
    void writeEncrypted (QByteArray& in);

    /**
     * Attempts to read a single message from the specified stream.
     *
     * @param available the number of available bytes to read.
     * @return true if we processed a message, false if we're still waiting for more data.
     */
    bool maybeReadMessage (QDataStream& stream, qint64 available);

    /** The server application. */
    ServerApp* _app;

    /** The underlying socket. */
    QTcpSocket* _socket;

    /** The stored address. */
    QHostAddress _address;

    /** The data stream used to read from and write to the socket. */
    QDataStream _stream;

    /** The encryption and decryption contexts. */
    EVP_CIPHER_CTX _ectx, _dctx;

    /** The display size reported by the client. */
    QSize _displaySize;

    /** The query arguments supplied to the client. */
    QMultiHash<QString, QString> _query;

    /** The cookie values stored on the client. */
    QHash<QString, QString> _cookies;

    /** Whether or not we're encrypting our messages. */
    bool _crypto;

    /** Whether or not the client is encrypting its messages. */
    bool _clientCrypto;
};

#endif // CONNECTION
