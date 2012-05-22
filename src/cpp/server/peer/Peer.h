//
// $Id$

#ifndef PEER
#define PEER

#include <QHash>

#include "db/PeerRepository.h"
#include "peer/AbstractPeer.h"

/**
 * Handles a single peer.
 */
class Peer : public AbstractPeer
{
    Q_OBJECT

public:

    /**
     * Initializes the peer.
     */
    Peer (ServerApp* app);

    /**
     * Destroys the peer.
     */
    virtual ~Peer ();

    /**
     * Returns a reference to the latest peer record.
     */
    const PeerRecord& record () const { return _record; }

    /**
     * Updates the peer with its most recent record.
     */
    void update (const PeerRecord& record);

    /**
     * Sends a request message to the peer.
     */
    template<class T> void sendRequestMessage (const T& msg, const Callback& callback);

    /**
     * Dispatches the response to a request.
     */
    void handleResponse (quint32 requestId, const QVariantList& args);

protected slots:

    /**
     * Attempts to reconnect, since we lost the connection.
     */
    void reconnectLater ();

    /**
     * Attempts to connect to the peer.
     */
    void connectToPeer ();

    /**
     * Sends our header, having established an encrypted connection.
     */
    void sendHeader ();

protected:

    /**
     * Handles an incoming message.
     */
    virtual void handle (const PeerMessage* message);

    /**
     * Returns the hostname to use when connecting to the peer.
     */
    const QString& hostname () const;

    /** The latest peer record. */
    PeerRecord _record;

    /** The last request id assigned. */
    quint32 _lastRequestId;

    /** Maps request ids to callbacks for pending requests. */
    QHash<quint32, Callback> _pendingRequests;
};

template<class T> inline void Peer::sendRequestMessage (const T& msg, const Callback& callback)
{
    T message = msg;
    _pendingRequests.insert(message.id = ++_lastRequestId, callback);
    sendMessage(msg);
}

#endif // PEER
