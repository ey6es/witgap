//
// $Id$

#ifndef PROTOCOL
#define PROTOCOL

#include <QtGlobal>

/**
 * Notes: the protocol is binary, message-based, and uses network byte order and Qt streaming.
 *
 * The client initiates communication by sending the preamble, which consists of:
 *     PROTOCOL_MAGIC : quint32
 *     PROTOCOL_VERSION : quint32
 *     sessionId : quint64 : the persistent session id
 *     sessionToken : char[16] : the randomly generated token for authenticating the session
 *     width : quint16 : the width of the client's display in characters
 *     height : quint16 : the height of the client's display in characters
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
 * The connected may be terminated at any time by the client (by sending WINDOW_CLOSED_MSG) or the
 * server (by sending CLOSE_MSG).
 */

/** The magic number that identifies the protocol. */
const quint32 PROTOCOL_MAGIC = 0x57544750; // "WTGP"

/** The protocol version. */
const quint32 PROTOCOL_VERSION = 0x00000001;

/**
 * Client -> server: the mouse was pressed.  Data:
 *     x : quint16 : the x location of the press
 *     y : quint16 : the y location of the press
 */
const quint8 MOUSE_PRESSED_MSG = 0;

/**
 * Client -> server: the mouse was released.  Data:
 *     x : quint16 : the x location of the press
 *     y : quint16 : the y location of the press
 */
const quint8 MOUSE_RELEASED_MSG = 1;

/**
 * Client -> server: a key was pressed.  Data:
 *     key : quint32 : the Qt key code
 */
const quint8 KEY_PRESSED_MSG = 2;

/**
 * Client -> server: a key was released.  Data:
 *     key : quint32 : the Qt key code
 */
const quint8 KEY_RELEASED_MSG = 3;

/**
 * Client -> server: the window was closed.  No data.
 */
const quint8 WINDOW_CLOSED_MSG = 4;

/**
 * Server -> client: add a window.  Data:
 *     id : qint32 : the unique window id
 *     layer : qint32 : the window layer
 *     x : qint16 : the x coordinate of the window
 *     y : qint16 : the y coordinate of the window
 *     width : qint16 : the width of the window
 *     height : qint16 : the height of the window
 *     fill : qint32 : the value with which to initialize the window contents
 */
const quint8 ADD_WINDOW_MSG = 0;

/**
 * Server -> client: remove a window.  Data:
 *     id : qint32 : the unique window id
 */
const quint8 REMOVE_WINDOW_MSG = 1;

/**
 * Server -> client: update a window.  Data:
 *     id : qint32 : the unique window id
 *     layer : qint32 : the window layer
 *     x : qint16 : the x coordinate of the window
 *     y : qint16 : the y coordinate of the window
 *     width : qint16 : the width of the window
 *     height : qint16 : the height of the window
 *     fill : qint32 : the value with which to initialize the exposed contents
 */
const quint8 UPDATE_WINDOW_MSG = 2;

/**
 * Server -> client: set part of a window's contents.  Data:
 *     id : qint32 : the unique window id
 *     x : qint16 : the x coordinate of the region to set
 *     y : qint16 : the y coordinate of the region to set
 *     width : qint16 : the width of the region to set
 *     height : qint16 : the height of the region to set
 *     contents : qint32[width*height] : the new contents of the region
 */
const quint8 SET_CONTENTS_MSG = 3;

/**
 * Server -> client: move part of a window's contents.  Data:
 *     id : qint32 : the unique window id
 *     x : qint16 : the x coordinate of the region to move
 *     y : qint16 : the y coordinate of the region to move
 *     width : qint16 : the width of the region to move
 *     height : qint16 : the height of the region to move
 *     destX : qint16 : the x coordinate of the new destination
 *     destY : qint16 : the y coordinate of the new destination
 *     fill : qint32 : the value with which to initialize the exposed contents
 */
const quint8 MOVE_CONTENTS_MSG = 4;

/**
 * Server -> client: set the session id/token.  Data:
 *     id : quint64 : the new persistent session id
 *     token : char[16] : the new randomly generated token
 */
const quint8 SET_SESSION_MSG = 5;

/**
 * Server -> client: a compound message follows.  The data is simply a series of normally-encoded
 * messages.
 */
const quint8 COMPOUND_MSG = 6;

/**
 * Server -> client: close the connection.  Data:
 *     reason : char[length - 1] : the UTF-8 encoded reason for the closure
 */
const quint8 CLOSE_MSG = 7;

/** A flag indicating that the character should be displayed in reverse. */
const int REVERSE_FLAG = 0x10000;

/**
 * Strips the flags from the specified character.
 */
inline int getChar (int ch) { return ch & 0xFFFF; }

#endif // PROTOCOL
