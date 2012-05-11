//
// $Id$

#ifndef PEER_PROTOCOL
#define PEER_PROTOCOL

#include <QMetaType>
#include <QVariant>
#include <QtGlobal>

#include "util/Streaming.h"

/**
 * Notes: the protocol is binary, message-based, and uses network byte order and Qt streaming.
 *
 * After the SSL handshake, the client initiates communication by sending the preamble, which
 * consists of:
 *     PeerProtocolMagic : quint32
 *     PeerProtocolVersion : quint32
 *     sharedSecret : char[sharedSecret.length()] : the secret phrase shared by peers
 *
 * If the server rejects the preamble, it will disconnect immediately.  Otherwise, both parties
 * may begin sending messages.
 *
 * Messages have the following format:
 *     length : quint32 : the length in bytes of the message (excluding this field)
 *     message : QVariant : a QVariant containing the message (a subclass of PeerMessage)
 *
 * The connected may be terminated at any time by the client or the server by sending a
 * CloseMessage.
 */

/** The magic number that identifies the protocol. */
const quint32 PeerProtocolMagic = 0x57545052; // "WTPR"

/** The protocol version. */
const quint32 PeerProtocolVersion = 0x00000001;

class Peer;
class PeerConnection;

/**
 * Base class for peer messages.
 */
class PeerMessage
{
public:

    /**
     * Handles a message sent from server to client.
     */
    virtual void handle (Peer* peer) const;

    /**
     * Handles a message sent from client to server.
     */
    virtual void handle (PeerConnection* connection) const;
};

/** Helper macro for upstream messages. */
#define UPSTREAM_MESSAGE public: \
    virtual void handle (PeerConnection* connection) const; \
    private:

/** Helper macro for downstream messages. */
#define DOWNSTREAM_MESSAGE public: \
    virtual void handle (Peer* peer) const; \
    private:

/** Helper macro for bidirectional messages. */
#define BIDIRECTIONAL_MESSAGE public: \
    virtual void handle (Peer* peer) const; \
    virtual void handle (PeerConnection* connection) const; \
    private:

/**
 * Closes the connection.
 */
class CloseMessage : public PeerMessage
{
    STREAMABLE
    BIDIRECTIONAL_MESSAGE
};

DECLARE_STREAMABLE_METATYPE(CloseMessage)

/**
 * Executes an action on the peer.
 */
class ExecuteMessage : public PeerMessage
{
    STREAMABLE
    UPSTREAM_MESSAGE

public:

    /** The action to perform. */
    STREAM QVariant action;
};

DECLARE_STREAMABLE_METATYPE(ExecuteMessage)

/**
 * Makes a request of the peer.
 */
class RequestMessage : public PeerMessage
{
    STREAMABLE
    UPSTREAM_MESSAGE

public:

    /** The identifier to be sent along with the response. */
    STREAM quint32 id;

    /** The request to process. */
    STREAM QVariant request;
};

DECLARE_STREAMABLE_METATYPE(RequestMessage)

/**
 * Responds to a request.
 */
class ResponseMessage : public PeerMessage
{
    STREAMABLE
    DOWNSTREAM_MESSAGE

public:

    /** The identifier of the request to which we're responding. */
    STREAM quint32 requestId;

    /** The response arguments. */
    STREAM QVariantList args;
};

DECLARE_STREAMABLE_METATYPE(ResponseMessage)

#endif // PEER_PROTOCOL
