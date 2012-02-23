//
// $Id$

#include <iostream>

#include <QtDebug>
#include <QtEndian>

#include "Protocol.h"

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"

const QMetaMethod& Connection::addWindowMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("addWindow(int,int,QRect,int)"));
    return method;
}

const QMetaMethod& Connection::removeWindowMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("removeWindow(int)"));
    return method;
}

const QMetaMethod& Connection::updateWindowMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("updateWindow(int,int,QRect,int)"));
    return method;
}

const QMetaMethod& Connection::setContentsMetaMethod ()
{
    static QMetaMethod method = staticMetaObject.method(
        staticMetaObject.indexOfMethod("setContents(int,QRect,QIntVector)"));
    return method;
}

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->connectionManager()),
    _app(app),
    _socket(socket),
    _stream(socket)
{
    // take over ownership of the socket
    _socket->setParent(this);

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));
    connect(this, SIGNAL(windowClosed()), SLOT(deleteLater()));
}

void Connection::activate ()
{
    // connect the signal in order to read incoming messages
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));

    // call it manually in case we already have something to read
    readMessages();
}

void Connection::deactivate (const QString& reason)
{
    QByteArray rbytes = reason.toUtf8();
    _stream << (quint16)(1 + rbytes.length());
    _stream << CLOSE_MSG;
    _socket->write(rbytes);
    _socket->disconnectFromHost();
}

void Connection::addWindow (int id, int layer, const QRect& bounds, int fill)
{
    _stream << (quint16)21;
    _stream << ADD_WINDOW_MSG;
    _stream << (qint32)id;
    _stream << (qint32)layer;
    write(bounds);
    _stream << (qint32)fill;
}

void Connection::removeWindow (int id)
{
    _stream << (qint16)5;
    _stream << REMOVE_WINDOW_MSG;
    _stream << (qint32)id;
}

void Connection::updateWindow (int id, int layer, const QRect& bounds, int fill)
{
    _stream << (qint16)21;
    _stream << UPDATE_WINDOW_MSG;
    _stream << (qint32)id;
    _stream << (qint32)layer;
    write(bounds);
    _stream << (qint32)fill;
}

void Connection::setContents (int id, const QRect& bounds, const QIntVector& contents)
{
    int size = contents.size();

    _stream << (quint16)(9 + size*4);
    _stream << SET_CONTENTS_MSG;
    write(bounds);
    for (int ii = 0; ii < size; ii++) {
        _stream << (qint32)contents.at(ii);
    }
}

void Connection::moveContents (int id, const QRect& source, const QPoint& dest, int fill)
{
    _stream << (quint16)21;
    _stream << MOVE_CONTENTS_MSG;
    _stream << (qint32)id;
    write(source);
    _stream << (qint16)dest.x();
    _stream << (qint16)dest.y();
    _stream << (qint32)fill;
}

void Connection::setSession (quint64 id, const QByteArray& token)
{
    _stream << (quint16)25;
    _stream << SET_SESSION_MSG;
    _stream << id;
    _socket->write(token);
}

void Connection::readHeader ()
{
    // if we don't have the full header, wait until we do
    if (_socket->bytesAvailable() < 36) {
        return;
    }
    // check the magic number and version
    quint32 magic, version;
    _stream >> magic;
    if (magic != PROTOCOL_MAGIC) {
        qWarning() << "Invalid protocol magic number:" << magic << _socket->peerAddress();
        deactivate("Invalid magic number.");
        return;
    }
    _stream >> version;
    if (version != PROTOCOL_VERSION) {
        qWarning() << "Wrong protocol version:" << version << _socket->peerAddress();
        deactivate("Wrong protocol version.");
        return;
    }
    quint64 sessionId;
    _stream >> sessionId;
    QByteArray sessionToken = _socket->read(16);
    quint16 width, height;
    _stream >> width;
    _stream >> height;

    // disable the connection until we're ready to read subsequent messages
    _socket->disconnect(this);

    // notify the connection manager, which will create a session for the connection
    _app->connectionManager()->connectionEstablished(this, sessionId, sessionToken, width, height);
}

void Connection::readMessages ()
{
    // read as many messages as are available
    while (true) {
        qint64 available = _socket->bytesAvailable();
        if (available < 1) {
            return; // wait until we have the rest of the header
        }
        char type;
        _socket->getChar(&type);
        if (type == WINDOW_CLOSED_MSG) {
            emit windowClosed();

        } else {
            if (available < 5) {
                _socket->ungetChar(type);
                return; // wait until we have the data
            }
            if (type == MOUSE_PRESSED_MSG || type == MOUSE_RELEASED_MSG) {
                quint16 x, y;
                _stream >> x;
                _stream >> y;
                emit (type == MOUSE_PRESSED_MSG) ? mousePressed(x, y) : mouseReleased(x, y);

            } else { // type == KEY_PRESSED_MSG || type == KEY_RELEASED_MSG
                quint32 key;
                _stream >> key;
                emit (type == KEY_PRESSED_MSG) ? keyPressed(key) : keyReleased(key);
            }
        }
    }
}

void Connection::write (const QRect& rect)
{
    _stream << (qint16)rect.left();
    _stream << (qint16)rect.top();
    _stream << (qint16)rect.width();
    _stream << (qint16)rect.height();
}
