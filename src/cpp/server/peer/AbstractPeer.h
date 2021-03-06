//
// $Id$

#ifndef ABSTRACT_PEER
#define ABSTRACT_PEER

#include <QDataStream>
#include <QIODevice>
#include <QObject>
#include <QVariant>

#include "util/Callback.h"

class QByteArray;
class QSslSocket;

class PeerMessage;
class ServerApp;

/**
 * Base class for {@link Peer} and {@link PeerConnection}.
 */
class AbstractPeer : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Encodes a message for transmission.
     */
    template<class T> static QByteArray encodeMessage (const T& message);

    /**
     * Initializes the peer.
     */
    AbstractPeer (ServerApp* app, QSslSocket* socket);

    /**
     * Destroys the peer.
     */
    virtual ~AbstractPeer ();

    /**
     * Returns the application pointer.
     */
    ServerApp* app () const { return _app; }

    /**
     * Encodes and sends a message to the connected peer.
     */
    template<class T> void sendMessage (const T& message) { sendMessage(encodeMessage(message)); }

    /**
     * Sends a pre-encoded message to the peer.
     */
    void sendMessage (const QByteArray& bytes);

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

    /**
     * Encodes a message for transmission.
     */
    static QByteArray encodeMessage (const QVariant& message);

    /** The server application. */
    ServerApp* _app;

    /** The socket through which we connect. */
    QSslSocket* _socket;

    /** The data stream for streaming. */
    QDataStream _stream;
};

template<class T> inline QByteArray AbstractPeer::encodeMessage (const T& message)
{
    return encodeMessage(QVariant::fromValue(message));
}

#endif // ABSTRACT_PEER
