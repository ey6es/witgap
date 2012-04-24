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
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"

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

    // start listening on the configured port
    QHostAddress address(app->config().value("listen_address").toString());
    quint16 port = app->config().value("listen_port").toInt();
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
    Session* session = _sessions[sessionId];
    if (session != 0) {
        if (session->record().token == sessionToken && session->connection() == 0) {
            // reconnection: use existing session
            session->setConnection(connection);
            return;
        }
        // invalid token or simultaneous connection: create a new session
        sessionId = 0;
    }

    // otherwise, go to the database to validate the token or generate a new one
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "validateToken",
        Q_ARG(quint64, sessionId), Q_ARG(const QByteArray&, sessionToken),
        Q_ARG(const Callback&, Callback(_this,
            "tokenValidated(QWeakObjectPointer,SessionRecord,UserRecord)",
            Q_ARG(const QWeakObjectPointer&, QWeakObjectPointer(connection)))));
}

void ConnectionManager::broadcast (const QString& speaker, const QString& message)
{
    foreach (Session* session, _sessions) {
        QMetaObject::invokeMethod(session->chatWindow(), "display",
            Q_ARG(const QString&, speaker), Q_ARG(const QString&, message),
            Q_ARG(ChatWindow::SpeakMode, ChatWindow::BroadcastMode));
    }
}

void ConnectionManager::tell (
    const QString& speaker, const QString& message,
    const QString& recipient, const Callback& callback)
{
    Session* session = _usernames.value(recipient.toLower());
    if (session == 0) {
        callback.invoke(Q_ARG(bool, false));
    } else {
        QMetaObject::invokeMethod(session->chatWindow(), "display",
            Q_ARG(const QString&, speaker), Q_ARG(const QString&, message),
            Q_ARG(ChatWindow::SpeakMode, ChatWindow::TellMode));
        callback.invoke(Q_ARG(bool, true));
    }
}

void ConnectionManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new Connection(_app, socket);
    }
}

void ConnectionManager::unmapSession (QObject* object)
{
    Session* session = static_cast<Session*>(object);
    _sessions.remove(session->record().id);
}

void ConnectionManager::tokenValidated (
    const QWeakObjectPointer& connptr, const SessionRecord& record, const UserRecord& user)
{
    // make sure the connection is still in business
    Connection* connection = static_cast<Connection*>(connptr.data());
    if (connection == 0 || !connection->isOpen()) {
        return;
    }

    // create and map the session
    Session* session = new Session(_app, connection, record, user);
    _sessions[record.id] = session;

    // listen for destruction in order to unmap
    connect(session, SIGNAL(destroyed(QObject*)), SLOT(unmapSession(QObject*)));
}
