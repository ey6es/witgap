//
// $Id$

#ifndef PROTOCOL
#define PROTOCOL

#include <QtGlobal>

/** The magic number that identifies the protocol. */
const quint32 PROTOCOL_MAGIC = 0x57544750; // "WTGP"

/** The protocol version. */
const quint32 PROTOCOL_VERSION = 0x00000001;

/** Types for messages sent from client to server. */
enum {

    /** Sent when a key is pressed. */
    KEY_PRESSED_MSG,

    /** Sent when a key is released. */
    KEY_RELEASED_MSG
};

/** Types for messages send from server to client. */
enum {

    /** Adds a window. */
    ADD_WINDOW_MSG,

    /** Removes a window. */
    REMOVE_WINDOW_MSG,

    /** Updates a window. */
    UPDATE_WINDOW_MSG,

    /** Sets part of a window's contents. */
    SET_CONTENTS_MSG,

    /** Moves part of a window's contents. */
    MOVE_CONTENTS_MSG,

    /** Sets the client's session id/token. */
    SET_SESSION_MSG,

    /** A message containing multiple sub-messages. */
    COMPOUND_MSG
};

#endif // PROTOCOL
