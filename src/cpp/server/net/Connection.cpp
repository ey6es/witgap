//
// $Id$

#include <iostream>

#include <QtDebug>
#include <QtEndian>

#include "Protocol.h"

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->connectionManager()),
    _app(app),
    _socket(socket)
{
    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
        SLOT(handleError(QAbstractSocket::SocketError)));
}

Connection::~Connection ()
{
}

void Connection::activate ()
{
    // connect the signal in order to read incoming messages
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));

    // call it manually in case we already have something to read
    readMessages();
}

void Connection::deactivate ()
{
}

void Connection::addWindow (int id, int layer, const QRect& bounds, int fill)
{
}

void Connection::removeWindow (int id)
{
}

void Connection::updateWindow (int id, int layer, const QRect& bounds, int fill)
{
}

void Connection::setContents (int id, const QRect& bounds, const int* contents)
{
}

void Connection::moveContents (int id, const QRect& source, const QPoint& dest, int fill)
{
}

void Connection::setSession (quint64 sessionId, const char* sessionToken)
{
}

void Connection::readHeader ()
{
    // if we don't have the full header, wait until we do
    if (_socket->bytesAvailable() < 36) {
        return;
    }
    // check the magic number and version
    QDataStream stream(_socket);
    quint32 magic, version;
    stream >> magic;
    if (magic != PROTOCOL_MAGIC) {
        qWarning() << "Invalid protocol magic number:" << magic << _socket->peerAddress();
        _socket->close();
        return;
    }
    stream >> version;
    if (version != PROTOCOL_VERSION) {
        qWarning() << "Wrong protocol version:" << version << _socket->peerAddress();
        _socket->close();
        return;
    }
    quint64 sessionId;
    stream >> sessionId;
    QByteArray sessionToken = _socket->read(16);
    quint16 width, height;
    stream >> width;
    stream >> height;

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
        if (available < 2) {
            return; // wait until we have the rest of the header
        }
        char sbuf[2];
        _socket->peek(sbuf, 2);
        quint16 size = qFromBigEndian<quint16>((uchar*)sbuf);
        if (available < size + 4) {
            return; // wait until we have the rest of the size
        }
        _socket->read(sbuf, 2); // consume the size we already read
        QByteArray message = _socket->read(size);
    }
}

void Connection::handleError (QAbstractSocket::SocketError error)
{
    qDebug() << "error" << error;
}
