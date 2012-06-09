//
// $Id$

#include <QSslSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/PeerConnection.h"
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
    // note the connection closing and unmap
    PeerManager* manager = qobject_cast<PeerManager*>(parent());
    if (manager != 0) {
        foreach (const SessionInfoPointer& ptr, _sessions) {
            manager->sessionRemoved(ptr->id);
        }
        foreach (const InstanceInfoPointer& ptr, _instances) {
            manager->instanceRemoved(ptr->id);
        }
        manager->freeInstanceIds(_reservedInstanceIds);
        manager->connectionClosed(this);
    }

    // log the destruction
    QString error;
    QDebug base = qDebug() << "Peer connection closed." << _name << _address;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
}

int PeerConnection::load () const
{
    return _sessions.size();
}

void PeerConnection::sessionAdded (const SessionInfoPointer& ptr)
{
    _sessions.insert(ptr->id, ptr);
}

void PeerConnection::sessionRemoved (const SessionInfoPointer& ptr)
{
    _sessions.remove(ptr->id);
}

void PeerConnection::instanceAdded (const InstanceInfoPointer& ptr)
{
    _reservedInstanceIds.remove(ptr->id);
    _instances.insert(ptr->id, ptr);
}

void PeerConnection::instanceRemoved (const InstanceInfoPointer& ptr)
{
    _instances.remove(ptr->id);
}

void PeerConnection::instanceIdReserved (quint64 id)
{
    _reservedInstanceIds.insert(id);
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
    qint64 available = _socket->bytesAvailable();
    if (available < 12) {
        return;
    }
    // check the magic number and version
    quint32 magic, version, length;
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
    // if we don't have the rest, wait until we do
    _stream >> length;
    if (available - 12 < length) {
        unget(_socket, length);
        unget(_socket, version);
        unget(_socket, magic);
        return;
    }
    QString secret;
    _stream >> secret;
    if (secret != _app->peerManager()->sharedSecret()) {
        qWarning() << "Wrong peer secret:" << secret << _address;
        _socket->disconnectFromHost();
        return;
    }
    _stream >> _name;

    // map
    _app->peerManager()->connectionEstablished(this);

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
