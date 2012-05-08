//
// $Id$

#include <QSslSocket>
#include <QTimer>

#include "ServerApp.h"
#include "peer/Peer.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"

Peer::Peer (ServerApp* app) :
    AbstractPeer(app, new QSslSocket())
{
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(reconnectLater()));
    connect(_socket, SIGNAL(encrypted()), SLOT(sendHeader()));
}

Peer::~Peer ()
{
    // log the destruction
    qDebug() << "Disconnected from peer." << _record.name;
}

void Peer::update (const PeerRecord& record)
{
    QString ohostname = hostname();
    quint16 oport = _record.port;

    _record = record;

    QString nhostname = hostname();
    if (ohostname != nhostname || oport != _record.port) {
        connectToPeer();
    }
}

void Peer::reconnectLater ()
{
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        qDebug() << "Peer connection error." << _record.name << _socket->errorString();
    }
    QTimer::singleShot(5000, this, SLOT(connectToPeer()));
}

void Peer::connectToPeer ()
{
    QString hostname = this->hostname();
    qDebug() << "Connecting to peer." << _record.name << hostname << _record.port;
    _socket->connectToHostEncrypted(hostname, _record.port);
}

void Peer::sendHeader ()
{
    _stream << PeerProtocolMagic;
    _stream << PeerProtocolVersion;
    _socket->write(_app->peerManager()->sharedSecret());
}

void Peer::handle (const PeerMessage* message)
{
    message->handle(this);
}

const QString& Peer::hostname () const
{
    return (_app->peerManager()->record().region == _record.region) ?
        _record.internalHostname : _record.externalHostname;
}
