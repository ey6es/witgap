//
// $Id$

#include <QSslSocket>
#include <QTimer>

#include "ServerApp.h"
#include "net/ConnectionManager.h"
#include "peer/Peer.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"

Peer::Peer (ServerApp* app) :
    AbstractPeer(app, new QSslSocket()),
    _lastRequestId(0)
{
    connect(_socket, SIGNAL(readyRead()), SLOT(readMessages()));
    connect(_socket, SIGNAL(error(QAbstractSocket::SocketError)), SLOT(reconnectLater()));
    connect(_socket, SIGNAL(encrypted()), SLOT(sendHeader()));
}

Peer::~Peer ()
{
    // unmap
    PeerManager* manager = qobject_cast<PeerManager*>(parent());
    if (manager != 0) {
        manager->peerDestroyed(this);
    }

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

void Peer::handleResponse (quint32 requestId, const QVariantList& args)
{
    QGenericArgument cargs[10];
    for (int ii = 0, nn = args.size(); ii < nn; ii++) {
        cargs[ii] = QGenericArgument(args.at(ii).typeName(), args.at(ii).constData());
    }
    _pendingRequests.take(requestId).invoke(
        cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
        cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
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

    const QString& secret = _app->peerManager()->sharedSecret();
    const QString& name = _app->peerManager()->record().name;
    const QString& region = _app->peerManager()->record().region;
    const QString& host = _app->peerManager()->record().externalHostname;
    _stream << (quint32)(18 + (secret.length() + name.length() +
        region.length() + host.length())*sizeof(QChar));
    _stream << secret;
    _stream << name;
    _stream << region;
    _stream << host;
    _stream << _app->connectionManager()->serverPort();

    // send our instance list
    InvokeAction action;
    action.sharedObjectId = _app->peerManager()->sharedObjectId();
    action.methodIndex = PeerManager::staticMetaObject.indexOfMethod(
        "instanceAdded(InstanceInfo)");
    action.args.append(QVariant());
    QVariant& variant = action.args[0];
    ExecuteMessage msg;
    foreach (const InstanceInfoPointer& ptr, _app->peerManager()->localInstances()) {
        variant.setValue(*ptr);
        msg.action.setValue(action);
        sendMessage(msg);
    }

    // and our session list
    action.methodIndex = PeerManager::staticMetaObject.indexOfMethod("sessionAdded(SessionInfo)");
    foreach (const SessionInfoPointer& ptr, _app->peerManager()->localSessions()) {
        variant.setValue(*ptr);
        msg.action.setValue(action);
        sendMessage(msg);
    }
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
