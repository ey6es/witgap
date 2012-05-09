//
// $Id$

#include <QFile>
#include <QMetaObject>
#include <QSslCipher>
#include <QSslKey>
#include <QSslSocket>
#include <QTimer>
#include <QVariant>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "peer/Peer.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"

void PeerManager::appendArguments (ArgumentDescriptorList* args)
{
    args->append("name", "The peer name.", "local", QVariant::String);
    args->append("region", "The geographic region.", "us-east", QVariant::String);
    args->append("internal_hostname", "The hostname within the region.",
        "localhost", QVariant::String);
    args->append("external_hostname", "The hostname outside the region.",
        "localhost", QVariant::String);
}

/** The interval at which we refresh the peer list. */
static const int PeerRefreshInterval = 60 * 1000;

/** The cutoff after which we consider a peer to be dead to the world. */
static const int LivingPeerCutoff = 5 * PeerRefreshInterval;

PeerManager::PeerManager (ServerApp* app) :
    QTcpServer(app),
    _app(app),
    _this(this)
{
    // initialize the peer record
    _record.name = app->args().value("name").toString();
    _record.region = app->args().value("region").toString();
    _record.internalHostname = app->args().value("internal_hostname").toString();
    _record.externalHostname = app->args().value("external_hostname").toString();
    _record.port = app->config().value("peer_listen_port").toInt() +
        app->args().value("port_offset").toInt();
    _record.active = true;

    // get the shared secret
    _sharedSecret = app->config().value("peer_secret").toByteArray();

    // prepare the shared SSL configuration
    QFile cert(app->config().value("certificate").toString());
    if (cert.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _sslConfig.setLocalCertificate(QSslCertificate(&cert));
        if (_sslConfig.localCertificate().isNull()) {
            qCritical() << "Invalid certificate file." << cert.fileName();
        }
    } else {
        qCritical() << "Missing certificate file." << cert.fileName();
    }
    QFile key(app->config().value("private_key").toString());
    if (key.open(QIODevice::ReadOnly | QIODevice::Text)) {
        _sslConfig.setPrivateKey(QSslKey(&key, QSsl::Rsa));
        if (_sslConfig.privateKey().isNull()) {
            qCritical() << "Invalid private key file." << key.fileName();
        }
    } else {
        qCritical() << "Missing private key file." << key.fileName();
    }
    _sslConfig.setCiphers(QSslSocket::supportedCiphers());
    _expectedSslErrors.append(QSslError(
        QSslError::SelfSignedCertificate, _sslConfig.localCertificate()));

    // start listening on the configured port
    QHostAddress address(app->config().value("peer_listen_address").toString());
    if (!listen(address, _record.port)) {
        qCritical() << "Failed to open peer server socket:" << errorString();
        return;
    }

    // enqueue an activation update
    QMetaObject::invokeMethod(app->databaseThread()->peerRepository(), "storePeer",
        Q_ARG(const PeerRecord&, _record));

    // deactivate when the application is exiting
    connect(app, SIGNAL(aboutToQuit()), SLOT(deactivate()));

    // perform initial refresh after a short delay (to allow peers to insert their records)
    QTimer::singleShot(5000, this, SLOT(refreshPeers()));

    // the usual refresh happens every minute
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(refreshPeers()));
    timer->start(PeerRefreshInterval);
}

void PeerManager::configureSocket (QSslSocket* socket) const
{
    socket->setSslConfiguration(_sslConfig);
    socket->ignoreSslErrors(_expectedSslErrors);
}

void PeerManager::execute (const QVariant& action)
{
    ExecuteMessage msg;
    msg.action = action;
    QByteArray bytes = AbstractPeer::encodeMessage(msg);
    foreach (Peer* peer, _peers) {
        peer->sendMessage(bytes);
    }
}

void PeerManager::refreshPeers ()
{
    // enqueue an update for our own record
    QMetaObject::invokeMethod(_app->databaseThread()->peerRepository(), "storePeer",
        Q_ARG(const PeerRecord&, _record));

    // load everyone else's
    QMetaObject::invokeMethod(_app->databaseThread()->peerRepository(), "loadPeers",
        Q_ARG(const Callback&, Callback(_this, "updatePeers(PeerRecordList)")));
}

void PeerManager::unmapPeer (QObject* object)
{
    Peer* peer = static_cast<Peer*>(object);
    _peers.remove(peer->record().name);
}

void PeerManager::deactivate ()
{
    // note in the database that we're no longer active
    _record.active = false;
    QMetaObject::invokeMethod(_app->databaseThread()->peerRepository(), "storePeer",
        Q_ARG(const PeerRecord&, _record));
}

void PeerManager::incomingConnection (int socketDescriptor)
{
    QSslSocket* socket = new QSslSocket(this);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        new PeerConnection(_app, socket);

    } else {
        qWarning() << "Invalid socket descriptor." << socketDescriptor;
        delete socket;
    }
}

void PeerManager::updatePeers (const PeerRecordList& records)
{
    QDateTime cutoff = QDateTime::currentDateTime().addMSecs(-LivingPeerCutoff);
    foreach (const PeerRecord& record, records) {
        if (record.name == _record.name) {
            continue; // it's our own record
        }
        if (record.active && record.updated > cutoff) {
            Peer*& peer = _peers[record.name];
            if (peer == 0) {
                peer = new Peer(_app);

                // listen for destruction in order to unmap
                connect(peer, SIGNAL(destroyed(QObject*)), SLOT(unmapPeer(QObject*)));
            }
            peer->update(record);

        } else {
            Peer* peer = _peers.take(record.name);
            if (peer != 0) {
                delete peer;
            }
        }
    }
}