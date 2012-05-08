//
// $Id$

#include <QMetaType>
#include <QSslSocket>

#include "ServerApp.h"
#include "peer/AbstractPeer.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"
#include "util/General.h"

AbstractPeer::AbstractPeer (ServerApp* app, QSslSocket* socket) :
    QObject(app->peerManager()),
    _app(app),
    _socket(socket),
    _stream(socket)
{
    // take over ownership of the socket and configure
    _socket->setParent(this);
    app->peerManager()->configureSocket(_socket);
}

AbstractPeer::~AbstractPeer ()
{
    // if we're still connected, we need to send a close message
    if (_socket->state() == QAbstractSocket::ConnectedState) {
        CloseMessage msg;
        sendMessage(&msg);
        _socket->flush();
    }
}

void AbstractPeer::sendMessage (const PeerMessage* message)
{
    // write the message to a buffer
    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    int type = message->type();
    out << type;
    QMetaType::save(out, type, message);

    // write the buffer to the socket
    quint32 length = bytes.length();
    _stream << length;
    _socket->write(bytes);
}

void AbstractPeer::readMessages ()
{
    // read as many messages as are available
    while (true) {
        qint64 available = _socket->bytesAvailable();
        if (available < 4) {
            return; // wait until we have the message length
        }
        quint32 length;
        _stream >> length;
        if (available < 4 + length) {
            unget(_socket, length);
            return; // wait until we have the entire message
        }
        int type;
        _stream >> type;
        void* data = QMetaType::construct(type);
        QMetaType::load(_stream, type, data);
        handle(static_cast<PeerMessage*>(data));
        QMetaType::destroy(type, data);
    }
}
