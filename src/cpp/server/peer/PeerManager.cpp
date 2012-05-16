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
    _lastRequestId(0),
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
    _sharedSecret = app->config().value("peer_secret").toString();

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

    // register for remote invocation
    addInvocationTarget(this);

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

void PeerManager::invoke (QObject* object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    Callback* callback;
    QVariant variant = prepareInvoke(
        object, method, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, &callback);
    if (callback == 0) {
        QMetaObject::invokeMethod(this, "execute", Q_ARG(const QVariant&, variant));

    } else {
        QMetaObject::invokeMethod(this, "request", Q_ARG(const QVariant&, variant),
            Q_ARG(const Callback&, *callback));
    }
}

void PeerManager::invoke (const QString& peer, QObject* object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    Callback* callback;
    QVariant variant = prepareInvoke(
        object, method, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, &callback);
    if (callback == 0) {
        QMetaObject::invokeMethod(this, "execute", Q_ARG(const QString&, peer),
            Q_ARG(const QVariant&, variant));

    } else {
        QMetaObject::invokeMethod(this, "request", Q_ARG(const QString&, peer),
            Q_ARG(const QVariant&, variant), Q_ARG(const Callback&, *callback));
    }
}

void PeerManager::invokeSession (const QString& name, QObject* object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9)
{
    Callback* callback;
    QVariant variant = prepareInvoke(
        object, method, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9, &callback);
    if (callback == 0) {
        QMetaObject::invokeMethod(this, "executeSession", Q_ARG(const QString&, name),
            Q_ARG(const QVariant&, variant));

    } else {
        QMetaObject::invokeMethod(this, "requestSession", Q_ARG(const QString&, name),
            Q_ARG(const QVariant&, variant), Q_ARG(const Callback&, *callback));
    }
}

void PeerManager::peerDestroyed (Peer* peer)
{
    _peers.remove(peer->record().name);
}

void PeerManager::connectionEstablished (PeerConnection* connection)
{
    _connections.insert(connection->name(), connection);
}

void PeerManager::connectionClosed (PeerConnection* connection)
{
    _connections.remove(connection->name());
}

void PeerManager::sessionAdded (const SessionInfo& info)
{
    SessionInfoPointer ptr(new SessionInfo(info));
    _sessions.insert(info.id, ptr);
    _sessionsByName.insert(info.name.toLower(), ptr);

    if (info.peer == _record.name) {
        _localSessions.insert(info.id, ptr);
    } else {
        _connections.value(info.peer)->sessionAdded(ptr);
    }
}

void PeerManager::sessionUpdated (const SessionInfo& info)
{
    SessionInfoPointer ptr = _sessions.value(info.id);
    QString oname = ptr->name.toLower();
    QString nname = (*ptr = info).name.toLower();

    // remap under new name if necessary
    if (oname != nname) {
        _sessionsByName.remove(oname);
        _sessionsByName.insert(nname, ptr);
    }
}

void PeerManager::sessionRemoved (quint64 id)
{
    SessionInfoPointer ptr = _sessions.take(id);
    _sessionsByName.remove(ptr->name.toLower());

    if (ptr->peer == _record.name) {
        _localSessions.remove(id);
    } else {
        _connections.value(ptr->peer)->sessionRemoved(ptr);
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

QVariant PeerManager::prepareInvoke (QObject* object, const char* method,
    QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7,
    QGenericArgument val8, QGenericArgument val9, Callback** callback)
{
    QGenericArgument args[] = { val0, val1, val2, val3, val4, val5, val6, val7, val8, val9 };
    QVariantList vargs;
    *callback = 0;
    for (int ii = 0; ii < 10 && args[ii].name() != 0; ii++) {
        int type = QMetaType::type(args[ii].name());
        if (type == Callback::Type) {
            *callback = static_cast<Callback*>(args[ii].data());
        } else {
            vargs.append(QVariant(type, args[ii].data()));
        }
    }
    // the presence of a callback determines whether we issue an action or a request
    if (*callback == 0) {
        InvokeAction action;
        action.targetIndex = invocationTargetIndex(object);
        action.methodIndex = object->metaObject()->indexOfMethod(method);
        action.args = vargs;
        return QVariant::fromValue(action);

    } else {
        InvokeRequest request;
        request.targetIndex = invocationTargetIndex(object);
        request.methodIndex = object->metaObject()->indexOfMethod(method);
        request.args = vargs;
        return QVariant::fromValue(request);
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

void PeerManager::execute (const QVariant& action)
{
    // encode and send it off to everyone else
    ExecuteMessage msg;
    msg.action = action;
    QByteArray bytes = AbstractPeer::encodeMessage(msg);
    foreach (Peer* peer, _peers) {
        peer->sendMessage(bytes);
    }

    // then run it here
    const PeerAction* paction = static_cast<const PeerAction*>(action.constData());
    paction->execute(_app);
}

void PeerManager::request (const QVariant& request, const Callback& callback)
{
    // store it
    quint32 requestId = ++_lastRequestId;
    PendingRequest& req = _pendingRequests[requestId];
    req.callback = callback;

    // send it off to everyone else
    foreach (Peer* peer, _peers) {
        req.remaining.insert(peer->record().name);
        peer->sendRequest(request, Callback(_this, "handleResponse(quint32,QString,QVariantList)",
            Q_ARG(quint32, requestId), Q_ARG(const QString&, peer->record().name)).setCollate());
    }

    // then it handle here
    req.remaining.insert(_record.name);
    const PeerRequest* prequest = static_cast<const PeerRequest*>(request.constData());
    prequest->handle(_app, Callback(_this, "handleResponse(quint32,QString,QVariantList)",
        Q_ARG(quint32, requestId), Q_ARG(const QString&, _record.name)).setCollate());
}

void PeerManager::execute (const QString& name, const QVariant& action)
{
    // see if it's for the local peer
    if (name == _record.name) {
        const PeerAction* paction = static_cast<const PeerAction*>(action.constData());
        paction->execute(_app);
        return;
    }
    Peer* peer = _peers.value(name);
    if (peer != 0) {
        ExecuteMessage msg;
        msg.action = action;
        peer->sendMessage(msg);
    }
}

void PeerManager::request (const QString& name, const QVariant& request, const Callback& callback)
{
    // see if it's for the local peer
    if (name == _record.name) {
        const PeerRequest* prequest = static_cast<const PeerRequest*>(request.constData());
        prequest->handle(_app, callback);
        return;
    }
    Peer* peer = _peers.value(name);
    if (peer != 0) {
        peer->sendRequest(request, callback);
    } else {
        callback.invokeWithDefaults();
    }
}

void PeerManager::executeSession (const QString& name, const QVariant& action)
{
    SessionInfoPointer ptr = _sessionsByName.value(name.toLower());
    if (!ptr.isNull()) {
        execute(ptr->peer, action);
    }
}

void PeerManager::requestSession (
    const QString& name, const QVariant& request, const Callback& callback)
{
    SessionInfoPointer ptr = _sessionsByName.value(name.toLower());
    if (!ptr.isNull()) {
        this->request(ptr->peer, request, callback);
    } else {
        callback.invokeWithDefaults();
    }
}

void PeerManager::handleResponse (quint32 requestId, const QString& name, const QVariantList& args)
{
    PendingRequest& req = _pendingRequests[requestId];
    req.results.insert(name, args);
    req.remaining.remove(name);
    if (req.remaining.isEmpty()) {
        req.callback.invoke(Q_ARG(const QVariantListHash&, req.results));
        _pendingRequests.remove(requestId);
    }
}

void InvokeAction::execute (ServerApp* app) const
{
    QObject* object = app->peerManager()->invocationTarget(targetIndex);
    QGenericArgument cargs[10];
    for (int ii = 0, nn = args.size(); ii < nn; ii++) {
        cargs[ii] = QGenericArgument(args[ii].typeName(), args[ii].constData());
    }
    object->metaObject()->method(methodIndex).invoke(object,
        cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
        cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
}

void InvokeRequest::handle (ServerApp* app, const Callback& callback) const
{
    QObject* object = app->peerManager()->invocationTarget(targetIndex);
    QMetaMethod method = object->metaObject()->method(methodIndex);
    QList<QByteArray> ptypes = method.parameterTypes();
    QGenericArgument cargs[10];
    bool passingCallback = false;

    // insert the callback into the arguments if appropriate
    for (int ii = 0, idx = 0, nn = ptypes.size(); ii < nn; ii++) {
        if (ptypes.at(ii) == "Callback") {
            cargs[ii] = Q_ARG(const Callback&, callback);
            passingCallback = true;

        } else {
            cargs[ii] = QGenericArgument(args[idx].typeName(), args[idx].constData());
            idx++;
        }
    }
    if (passingCallback) {
        method.invoke(object,
            cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
            cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
        return;
    }

    // if the method doesn't take a callback, use the return value (if any)
    if (method.typeName()[0] == 0) {
        method.invoke(object,
            cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
            cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
        callback.invoke();
        return;
    }
    int type = QMetaType::type(method.typeName());
    void* data = QMetaType::construct(type);
    QGenericReturnArgument returnValue(method.typeName(), data);
    method.invoke(object, returnValue,
        cargs[0], cargs[1], cargs[2], cargs[3], cargs[4],
        cargs[5], cargs[6], cargs[7], cargs[8], cargs[9]);
    callback.invoke(returnValue);
    QMetaType::destroy(type, data);
}
