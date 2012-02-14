//
// $Id$

#ifndef PROTOCOL
#define PROTOCOL

#include <QtGlobal>

/** The magic number that identifies the protocol. */
const quint32 PROTOCOL_MAGIC = 0x57544750; // "WTGP"

/** The protocol version. */
const quint32 PROTOCOL_VERSION = 0x00000001;

/** Client -> server: a key was pressed. */
const quint8 KEY_PRESSED_MSG = 0;

/** Client -> server: a key was released. */
const quint8 KEY_RELEASED_MSG = 1;

/** Client -> server: the window was closed. */
const quint8 WINDOW_CLOSED_MSG = 2;

/** Server -> client: add a window. */
const quint8 ADD_WINDOW_MSG = 0;

/** Server -> client: remove a window. */
const quint8 REMOVE_WINDOW_MSG = 1;

/** Server -> client: update a window. */
const quint8 UPDATE_WINDOW_MSG = 2;

/** Server -> client: set part of a window's contents. */
const quint8 SET_CONTENTS_MSG = 3;

/** Server -> client: move part of a window's contents. */
const quint8 MOVE_CONTENTS_MSG = 4;

/** Server -> client: set the session id/token. */
const quint8 SET_SESSION_MSG = 5;

/** Server -> client: a compound message follows. */
const quint8 COMPOUND_MSG = 6;

/** Server -> client: close the connection. */
const quint8 CLOSE_MSG = 7;

#endif // PROTOCOL
