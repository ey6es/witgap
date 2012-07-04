//
// $Id$

#ifndef PROTOCOL
#define PROTOCOL

#include <QtGlobal>

/**
 * Notes: the protocol is binary, message-based, and uses network byte order.
 *
 * The client initiates communication by sending the preamble, which consists of:
 *     PROTOCOL_MAGIC : quint32
 *     PROTOCOL_VERSION : quint32
 *     length : quint32 : the length in bytes of the remainder (excluding this field)
 *     width : quint16 : the width of the client's display in characters
 *     height : quint16 : the height of the client's display in characters
 *     encryptedSecret : char[128] : the secret key, encrypted with the public RSA key
 *
 *     The remainder of the preamble is encrypted with the secret key:
 *
 *     queryLength : quint16 : the length of the URL query string
 *     query : char[queryLength] : the UTF-8 encoded URL query string
 *     cookieLength : quint16 : the length of the cookie string
 *     cookie : char[cookieLength] : the UTF-8 encoded cookie string
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
 * server (by sending CLOSE_MSG/RECONNECT_MSG).
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
 *     char : quint16 : the Unicode character
 */
const quint8 KEY_PRESSED_MSG = 2;

/**
 * Client -> server: a key was pressed on the number pad.  Data:
 *     key : quint32 : the Qt key code
 *     char : quint16 : the Unicode character
 */
const quint8 KEY_PRESSED_NUMPAD_MSG = 3;

/**
 * Client -> server: a key was released.  Data:
 *     key : quint32 : the Qt key code
 *     char : quint16 : the Unicode character
 */
const quint8 KEY_RELEASED_MSG = 4;

/**
 * Client -> server: a key was released on the number pad.  Data:
 *     key : quint32 : the Qt key code
 *     char : quint16 : the Unicode character
 */
const quint8 KEY_RELEASED_NUMPAD_MSG = 5;

/**
 * Client -> server: the window was closed.  No data.
 */
const quint8 WINDOW_CLOSED_MSG = 6;

/**
 * Client -> server: encryption toggled.  No data.
 */
const quint8 CRYPTO_TOGGLED_MSG = 7;

/**
 * Client -> server: ping response.  Data:
 *     clock : quint64 : the server clock value
 */
const quint8 PONG_MSG = 8;

/**
 * Server -> client: add or update a window.  Data:
 *     id : qint32 : the unique window id
 *     layer : qint32 : the window layer
 *     x : qint16 : the x coordinate of the window
 *     y : qint16 : the y coordinate of the window
 *     width : qint16 : the width of the window
 *     height : qint16 : the height of the window
 *     fill : qint32 : the value with which to initialize the window contents
 */
const quint8 UPDATE_WINDOW_MSG = 0;

/**
 * Server -> client: remove a window.  Data:
 *     id : qint32 : the unique window id
 */
const quint8 REMOVE_WINDOW_MSG = 1;

/**
 * Server -> client: set part of a window's contents.  Data:
 *     id : qint32 : the unique window id
 *     x : qint16 : the x coordinate of the region to set
 *     y : qint16 : the y coordinate of the region to set
 *     width : qint16 : the width of the region to set
 *     height : qint16 : the height of the region to set
 *     contents : qint32[width*height] : the new contents of the region
 */
const quint8 SET_CONTENTS_MSG = 2;

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
const quint8 MOVE_CONTENTS_MSG = 3;

/**
 * Server -> client: set a client cookie.  Data:
 *     nameLength : quint16 : the length of the name string in bytes
 *     name : char[nameLength] : the UTF-8 encoded name string
 *     value : char[length - 3 - nameLength] : the UTF-8 encoded value string
 */
const quint8 SET_COOKIE_MSG = 4;

/**
 * Server -> client: close the connection.  Data:
 *     reason : char[length - 1] : the UTF-8 encoded reason for the closure
 */
const quint8 CLOSE_MSG = 5;

/**
 * Server -> client: toggle encryption.  No data.
 */
const quint8 TOGGLE_CRYPTO_MSG = 6;

/**
 * Server -> client: compound message.  The data is simply the sub-messages.
 */
const quint8 COMPOUND_MSG = 7;

/**
 * Server -> client: ping request.  Data:
 *     clock : quint64 : the server clock value
 */
const quint8 PING_MSG = 8;

/**
 * Server -> client: connect to a different peer.  Data:
 *     host : char[length - 3] : the UTF-8 encoded hostname
 *     port : quint16 : the port
 */
const quint8 RECONNECT_MSG = 9;

/**
 * Server -> client: evaluate an expression in the client Javascript context.  Data:
 *     expression : char[length - 1] : the UTF-8 encoded expression to evaluate
 */
const quint8 EVALUATE_MSG = 10;

/** A flag indicating that the character should be displayed in reverse. */
const int REVERSE_FLAG = 0x10000;

/** A flag indicating that the character should be displayed half-bright. */
const int DIM_FLAG = 0x20000;

/**
 * Strips the flags from the specified character.
 */
inline int getChar (int ch) { return ch & 0xFFFF; }

/**
 * Strips the value from the specified character.
 */
inline int getFlags (int ch) { return ch & 0xFFFF0000; }

#endif // PROTOCOL
