//
// $Id$

#ifndef PEER_CONNECTION
#define PEER_CONNECTION

#include <QHostAddress>

#include "peer/AbstractPeer.h"
#include "peer/PeerManager.h"

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

    /**
     * Returns a reference to the peer name.
     */
    const QString& name () const { return _name; }

    /**
     * Notes that a session has been added by this connection.
     */
    void sessionAdded (const SessionInfoPointer& ptr);

    /**
     * Notes that a session has been removed by this connection.
     */
    void sessionRemoved (const SessionInfoPointer& ptr);

    /**
     * Notes that an instance has been added by this connection.
     */
    void instanceAdded (const InstanceInfoPointer& ptr);

    /**
     * Notes that an instance has been removed by this connection.
     */
    void instanceRemoved (const InstanceInfoPointer& ptr);

    /**
     * Notes that an instance id has been reserved by this connection.
     */
    void instanceIdReserved (quint64 id);

    /**
     * Sends a response to a request.
     */
    Q_INVOKABLE void sendResponse (quint32 requestId, const QVariantList& args);

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

    /** The peer name. */
    QString _name;

    /** The sessions registered by this connection. */
    QHash<quint64, SessionInfoPointer> _sessions;

    /** The instances registered by this connection. */
    QHash<quint64, InstanceInfoPointer> _instances;

    /** The instance ids reserved by this connection. */
    QSet<quint64> _reservedInstanceIds;
};

#endif // PEER_CONNECTION
