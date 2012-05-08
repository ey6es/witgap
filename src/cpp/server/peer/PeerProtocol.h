//
// $Id$

#ifndef PEER_PROTOCOL
#define PEER_PROTOCOL

#include <QtGlobal>

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
 * Messages sent from client to server have the following format:
 *     type : quint8 : the message type, one of the constants enumerated below
 *     data : ... : the message data as described in the type documentation
 *
 * Messages sent from server to client have the following format:
 *     length : quint16 : the length in bytes of the message (excluding this field)
 *     type : quint8 : the message type, one of the constants enumerated below
 *     data : ... : the message data as described in the type documentation
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
     * Returns the message's type id.
     */
    virtual int type () const = 0;

    /**
     * Handles a message sent from server to client.
     */
    virtual void handle (Peer* peer) const;

    /**
     * Handles a message sent from client to server.
     */
    virtual void handle (PeerConnection* connection) const;
};

/**
 * Closes the connection.
 */
class CloseMessage : public PeerMessage
{
public:

    /**
     * Returns the message's type id.
     */
    virtual int type () const;

    /**
     * Handles a message sent from server to client.
     */
    virtual void handle (Peer* peer) const;

    /**
     * Handles a message sent from client to server.
     */
    virtual void handle (PeerConnection* connection) const;
};

#endif // PEER_PROTOCOL
