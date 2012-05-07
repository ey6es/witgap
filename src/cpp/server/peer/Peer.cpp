//
// $Id$

#include <QSslSocket>
#include <QTimer>

#include "ServerApp.h"
#include "peer/Peer.h"
#include "peer/PeerManager.h"

Peer::Peer (ServerApp* app) :
    QObject(app->peerManager()),
    _app(app),
    _socket(new QSslSocket(this))
{
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(reconnectLater()));
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
    QString error;
    QDebug base = qDebug() << "Disconnected from peer." << _record.name;
    if (_socket->error() != QAbstractSocket::UnknownSocketError) {
        base << _socket->errorString();
    }
    QTimer::singleShot(5000, this, SLOT(connectToPeer()));
}

void Peer::connectToPeer ()
{
    QString hostname = this->hostname();
    qDebug() << "Connecting to peer." << _record.name << hostname << _record.port;
    _socket->connectToHostEncrypted(hostname, _record.port);
}

const QString& Peer::hostname () const
{
    return (_app->peerManager()->record().region == _record.region) ?
        _record.internalHostname : _record.externalHostname;
}
