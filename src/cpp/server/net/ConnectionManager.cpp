//
// $Id$

#include <stdio.h>

#include <QTcpSocket>
#include <QtDebug>

#include <openssl/rsa.h>

#include "ServerApp.h"
#include "chat/ChatWindow.h"
#include "db/DatabaseThread.h"
#include "db/SessionRepository.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "peer/PeerManager.h"
#include "util/General.h"

ConnectionManager::ConnectionManager (ServerApp* app) :
    QTcpServer(app),
    _app(app),
    _rsa(0),
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
    _app->peerManager()->addInvocationTarget(this);

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
}

ConnectionManager::~ConnectionManager ()
{
    if (_rsa != 0) {
        RSA_free(_rsa);
    }
}

void ConnectionManager::connectionEstablished (Connection* connection)
{
    // see if we already have a session with the provided id
    quint32 sessionId = connection->cookies().value("sessionId", "0").toULongLong(0, 16);
    QByteArray sessionToken = QByteArray::fromHex(
        connection->cookies().value("sessionToken", "00000000000000000000000000000000").toAscii());
    Session* session = _sessions.value(sessionId);
    if (session != 0) {
        QMetaObject::invokeMethod(session, "maybeSetConnection",
            Q_ARG(const SharedConnectionPointer&, connection->pointer()),
            Q_ARG(const QByteArray&, sessionToken), Q_ARG(const Callback&, Callback(_this,
                "connectionMaybeSet(SharedConnectionPointer,bool)",
                Q_ARG(const SharedConnectionPointer&, connection->pointer()))));
        return;
    }

    // otherwise, go to the database to validate the token or generate a new one
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "validateToken",
        Q_ARG(quint64, sessionId), Q_ARG(const QByteArray&, sessionToken),
        Q_ARG(const Callback&, Callback(_this,
            "tokenValidated(SharedConnectionPointer,SessionRecord,UserRecord)",
            Q_ARG(const SharedConnectionPointer&, connection->pointer()))));
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

void ConnectionManager::sessionNameChanged (const QString& oldName, const QString& newName)
{
    Session* session = _names.take(oldName.toLower());
    _names.insert(newName.toLower(), session);
}

void ConnectionManager::sessionClosed (quint64 id, const QString& name)
{
    Session* session = _sessions.take(id);
    _names.remove(name.toLower());

    session->deleteLater();
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

    // create a new token
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "validateToken",
        Q_ARG(quint64, 0), Q_ARG(const QByteArray&, QByteArray()),
        Q_ARG(const Callback&, Callback(_this,
            "tokenValidated(SharedConnectionPointer,SessionRecord,UserRecord)",
            Q_ARG(const SharedConnectionPointer&, connptr))));
}

void ConnectionManager::tokenValidated (
    const SharedConnectionPointer& connptr, const SessionRecord& record, const UserRecord& user)
{
    // make sure the connection is still in business
    if (!connptr->isOpen()) {
        return;
    }

    // create and map the session
    Session* session = new Session(_app, connptr, record, user);
    _sessions.insert(record.id, session);
    _names.insert(record.name.toLower(), session);
}
