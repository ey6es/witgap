//
// $Id$

#include <QSslSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"

PeerConnection::PeerConnection (ServerApp* app, QSslSocket* socket) :
    AbstractPeer(app, socket)
{
    // start encryption
    _socket->startServerEncryption();

    // connect initial slots
    connect(socket, SIGNAL(readyRead()), SLOT(readHeader()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(deleteLater()));
    connect(socket, SIGNAL(disconnected()), SLOT(deleteLater()));

    // log the connection
    qDebug() << "Peer connection opened." << (_address = _socket->peerAddress());
}

PeerConnection::~PeerConnection ()
{
    // log the destruction
    QString error;
    QDebug base = qDebug() << "Peer connection closed." << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}

void PeerConnection::sendResponse (quint32 requestId, const QVariantList& args)
{
    ResponseMessage msg;
    msg.requestId = requestId;
    msg.args = args;
    sendMessage(msg);
}

void PeerConnection::readHeader ()
{
    // if we don't have the full header, wait until we do
    const QByteArray& secret = _app->peerManager()->sharedSecret();
    if (_socket->bytesAvailable() < 8 + secret.length()) {
        return;
    }
    // check the magic number and version
    quint32 magic, version;
    _stream >> magic;
    if (magic != PeerProtocolMagic) {
        qWarning() << "Invalid protocol magic number:" << magic << _address;
        _socket->disconnectFromHost();
        return;
    }
    _stream >> version;
    if (version != PeerProtocolVersion) {
        qWarning() << "Wrong protocol version:" << version << _address;
        _socket->disconnectFromHost();
        return;
    }
    QByteArray peerSecret = _socket->read(secret.length());
    if (peerSecret != secret) {
        qWarning() << "Wrong peer secret:" << peerSecret << _address;
        _socket->disconnectFromHost();
        return;
    }

    // switch over to reading incoming messages
    _socket->disconnect(this, SLOT(readHeader()));
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));

    // call it manually in case we already have something to read
    readMessages();
}

void PeerConnection::handle (const PeerMessage* message)
{
    message->handle(this);
}
