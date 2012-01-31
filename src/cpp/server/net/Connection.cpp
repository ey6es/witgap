//
// $Id$

#include <iostream>

#include <QtDebug>
#include <QtEndian>

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"

Connection::Connection (ServerApp* app, QTcpSocket* socket) :
    QObject(app->connectionManager()),
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

void Connection::readHeader ()
{
    // if we don't have the full header, wait until we do
    if (_socket->bytesAvailable() < 8) {
        return;
    }
    // check the magic number and version
    QDataStream stream(_socket);
    quint32 magic, version;
    stream >> magic;
    if (magic != 0x57544750) { // "WTGP"
        qWarning() << "Invalid protocol magic number:" << magic << _socket->peerAddress();
        _socket->close();
        return;
    }
    stream >> version;
    if (version != 0x00000001) {
        qWarning() << "Wrong protocol version:" << version << _socket->peerAddress();
        _socket->close();
        return;
    }
    // if that worked, we can switch to reading messages
    _socket->disconnect(this);
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));

    // call it manually in case we already have something to read
    readMessages();
}

void Connection::readMessages ()
{
    // read as many messages as are available
    while (true) {
        qint64 available = _socket->bytesAvailable();
        if (available < 4) {
            return; // wait until we have the rest of the header
        }
        char sbuf[4];
        _socket->peek(sbuf, 4);
        quint32 size = qFromBigEndian<quint32>((uchar*)sbuf);
        if (available < size + 4) {
            return; // wait until we have the rest of the size
        }
        _socket->read(sbuf, 4); // consume the size we already read
        QByteArray message = _socket->read(size);
    }
}

void Connection::handleError (QAbstractSocket::SocketError error)
{
    qDebug() << "error" << error;
}
