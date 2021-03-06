//
// $Id$

#include <stdio.h>

#include <QTcpSocket>
#include <QtDebug>

#include <openssl/rsa.h>

#include "ServerApp.h"
#include "chat/ChatWindow.h"
#include "db/DatabaseThread.h"
#include "db/UserRepository.h"
#include "http/HttpConnection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "peer/PeerConnection.h"
#include "util/General.h"

ConnectionManager::ConnectionManager (ServerApp* app) :
    QTcpServer(app),
    _app(app),
    _rsa(0),
    _geoIp(GeoIP_open(app->config().value("geoip_db").toByteArray().constData(), GEOIP_STANDARD)),
    _this(this)
{
    // read the private RSA key
    FILE* keyFile = fopen(app->config().value("private_key").toByteArray().constData(), "r");
    if (keyFile == 0) {
        qCritical() << "Couldn't open private key file.";
        return;
    }
    _rsa = PEM_read_RSAPrivateKey(keyFile, 0, 0, 0);
    if (_rsa == 0) {
        qCritical() << "Invalid private key file.";
        return;
    }

    // register for remote invocation
    _app->peerManager()->registerSharedObject(this);

    // start listening on the configured port
    QHostAddress address(app->config().value("listen_address").toString());
    quint16 port = app->config().value("listen_port").toInt() +
        app->args().value("port_offset").toInt();
    if (!listen(address, port)) {
        qCritical() << "Failed to open server socket:" << errorString();
        return;
    }

    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));

    // register for /client
    _app->httpManager()->registerSubhandler("client", this);
}

ConnectionManager::~ConnectionManager ()
{
    GeoIP_delete(_geoIp);
    if (_rsa != 0) {
        RSA_free(_rsa);
    }
}

void ConnectionManager::connectionEstablished (Connection* connection)
{
    // see if we already have a session with the provided id
    quint64 userId = connection->cookies().value("userId", "0").toULongLong(0, 16);
    QByteArray sessionToken = QByteArray::fromHex(
        connection->cookies().value("sessionToken", "00000000000000000000000000000000").toAscii());
    SessionInfoPointer ptr = _app->peerManager()->sessions().value(userId);
    if (ptr && !ptr->connected) {
        if (ptr->peer == _app->peerManager()->record().name) {
            // it's on this peer; try to set the connection directly
            QMetaObject::invokeMethod(_sessions.value(userId), "maybeSetConnection",
                Q_ARG(const SharedConnectionPointer&, connection->pointer()),
                Q_ARG(const QByteArray&, sessionToken), Q_ARG(const Callback&, Callback(_this,
                    "connectionMaybeSet(SharedConnectionPointer,bool)",
                    Q_ARG(const SharedConnectionPointer&, connection->pointer()))));

        } else {
            // it's on a remote peer; tell the connection to reconnect
            PeerConnection* pconn = _app->peerManager()->connections().value(ptr->peer);
            connection->reconnect(pconn->host(), pconn->portOffset());
        }
        return;
    }

    // otherwise, go to the database to validate the token or generate a new one
    QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "validateSessionToken",
        Q_ARG(quint64, userId), Q_ARG(const QByteArray&, sessionToken),
        Q_ARG(const Callback&, Callback(_this,
            "tokenValidated(SharedConnectionPointer,UserRecord)",
            Q_ARG(const SharedConnectionPointer&, connection->pointer()))));
}

void ConnectionManager::transferSession (const SessionTransfer& transfer)
{
    qDebug() << "Session transferred." << transfer.user.name;

    // create and map the session
    Session* session = new Session(_app, SharedConnectionPointer(), transfer.user, transfer);
    _sessions.insert(transfer.user.id, session);
    _names.insert(transfer.user.name.toLower(), session);
}

void ConnectionManager::broadcast (const QString& speaker, const QString& message)
{
    foreach (Session* session, _sessions) {
        QMetaObject::invokeMethod(session->chatWindow(), "display",
            Q_ARG(const QString&, speaker), Q_ARG(const QString&, message),
            Q_ARG(ChatWindow::SpeakMode, ChatWindow::BroadcastMode));
    }
}

void ConnectionManager::broadcast (const TranslationKey& key)
{
    foreach (Session* session, _sessions) {
        QMetaObject::invokeMethod(session->chatWindow(), "display",
            Q_ARG(const TranslationKey&, key));
    }
}

void ConnectionManager::tell (
    const QString& speaker, const QString& message,
    const QString& recipient, const Callback& callback)
{
    Session* session = _names.value(recipient.toLower());
    if (session != 0) {
        QMetaObject::invokeMethod(session->chatWindow(), "display",
            Q_ARG(const QString&, speaker), Q_ARG(const QString&, message),
            Q_ARG(ChatWindow::SpeakMode, ChatWindow::TellMode));
        callback.invoke(Q_ARG(bool, true));

    } else {
        callback.invoke(Q_ARG(bool, false));
    }
}

void ConnectionManager::summon (
    const QString& name, const QString& summoner, const Callback& callback)
{
    Session* session = _names.value(name.toLower());
    if (session != 0) {
        QMetaObject::invokeMethod(session, "moveToPlayer", Q_ARG(const QString&, summoner));
        callback.invoke(Q_ARG(bool, true));

    } else {
        callback.invoke(Q_ARG(bool, false));
    }
}

void ConnectionManager::sessionChanged (
    quint64 oldId, quint64 newId, const QString& oldName, const QString& newName)
{
    Session* session = _sessions.take(oldId);
    _sessions.insert(newId, session);
    _names.remove(oldName.toLower());
    _names.insert(newName.toLower(), session);
}

void ConnectionManager::sessionClosed (quint64 id, const QString& name)
{
    Session* session = _sessions.take(id);
    _names.remove(name.toLower());

    session->deleteLater();
}

bool ConnectionManager::handleRequest (
    HttpConnection* connection, const QString& name, const QString& path)
{
    if (connection->isWebSocketRequest()) {
        connection->switchToWebSocket();
        new Connection(_app, connection);
        return true;
    }
    return false;
}

void ConnectionManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new Connection(_app, socket);
    }
}

void ConnectionManager::connectionMaybeSet (const SharedConnectionPointer& connptr, bool success)
{
    // make sure we failed to install the connection and the connection is still in business
    if (success || !connptr->isOpen()) {
        return;
    }

    // create a new user
    QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "createUser",
        Q_ARG(const Callback&, Callback(_this,
            "tokenValidated(SharedConnectionPointer,UserRecord)",
            Q_ARG(const SharedConnectionPointer&, connptr))));
}

void ConnectionManager::tokenValidated (
    const SharedConnectionPointer& connptr, const UserRecord& user)
{
    // make sure the connection is still in business
    if (!connptr->isOpen()) {
        return;
    }

    // set the user id and token cookies
    connptr->setCookie("userId", QString::number(user.id, 16).rightJustified(16, '0'));
    connptr->setCookie("sessionToken", user.sessionToken.toHex());

    // create and map the session
    Session* session = new Session(_app, connptr, user, SessionTransfer());
    _sessions.insert(user.id, session);
    _names.insert(user.name.toLower(), session);
}
