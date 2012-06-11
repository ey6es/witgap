//
// $Id$

#ifndef CONNECTION
#define CONNECTION

#include <QHash>
#include <QHostAddress>
#include <QMultiHash>
#include <QSharedPointer>
#include <QSize>
#include <QTcpSocket>

#include <openssl/evp.h>

#include <GeoIPCity.h>

#include "util/General.h"

class QMetaMethod;
class QIntVector;
class QPoint;
class QRect;

class Connection;
class ServerApp;

/** A shared connection pointer that we'll register with the metatype system. */
typedef QSharedPointer<Connection> SharedConnectionPointer;

/**
 * Handles a single TCP connection.
 */
class Connection : public QObject
{
    Q_OBJECT

public:

    /**
     * Ensures that all messages are compounded when in scope.
     */
    class Compounder
    {
    public:

        /**
         * Creates a new compounder for the specified connection.
         */
        Compounder (Connection* connection);

        /**
         * Destroys the compounder.
         */
        ~Compounder ();

    protected:

        /** The connection whose messages should be compounded. */
        Connection* _connection;
    };

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
     * Returns the meta-method for {@link #startCompound}.
     */
    static const QMetaMethod& startCompoundMetaMethod ();

    /**
     * Returns the meta-method for {@link #commitCompound}.
     */
    static const QMetaMethod& commitCompoundMetaMethod ();

    /**
     * Returns the meta-method for {@link #reconnect}.
     */
    static const QMetaMethod& reconnectMetaMethod ();

    /**
     * Returns the meta-method for {@link #evaluate}.
     */
    static const QMetaMethod& evaluateMetaMethod ();

    /**
     * Initializes the connection.
     */
    Connection (ServerApp* app, QTcpSocket* socket);

    /**
     * Destroys the connection.
     */
    ~Connection ();

    /**
     * Returns a reference to the shared connection pointer (cleared when the connection is
     * closed).
     */
    const SharedConnectionPointer& pointer () const { return _pointer; }

    /**
     * Checks whether the connection is open.
     */
    bool isOpen () const { return _pointer; }

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
    Q_INVOKABLE void activate ();

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

    /**
     * Starts a compound message.
     */
    Q_INVOKABLE void startCompound ();

    /**
     * Commits a compound message.
     */
    Q_INVOKABLE void commitCompound ();

    /**
     * Tells the client to reconnect to a different peer.
     */
    Q_INVOKABLE void reconnect (const QString& host, quint16 port);

    /**
     * Tells the client to evaluate an expression in its Javascript context.
     */
    Q_INVOKABLE void evaluate (const QString& expression);

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

    /**
     * Fired when the connection is closed.
     */
    void closed ();

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
     * Pings the client.
     */
    void ping ();

    /**
     * Closes the connection.
     */
    void close ();

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

    /** A shared pointer to the connection itself. */
    SharedConnectionPointer _pointer;

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

    /** The client's GeoIP info. */
    GeoIPRecord* _geoIpRecord;

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

    /** The number of compound requests.  When it drops to zero, we can submit the compound. */
    int _compoundCount;

    /** The incoming message throttle. */
    Throttle _throttle;
};

#endif // CONNECTION
