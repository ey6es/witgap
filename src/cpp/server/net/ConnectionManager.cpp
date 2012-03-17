//
// $Id$

#include <QTcpSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SessionRepository.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"

ConnectionManager::ConnectionManager (ServerApp* app) :
    QTcpServer(app),
    _app(app),
    _this(this)
{
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

void ConnectionManager::connectionEstablished (
    Connection* connection, quint64 sessionId, const QByteArray& sessionToken)
{
    // see if we already have a session with the provided id
    Session* session = _sessions[sessionId];
    if (session != 0) {
        if (session->token() == sessionToken && session->connection() == 0) {
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
            "tokenValidated(QWeakObjectPointer,quint64,QByteArray,UserRecord)",
            Q_ARG(const QWeakObjectPointer&, QWeakObjectPointer(connection)))));
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
    _sessions.remove(session->id());
}

void ConnectionManager::tokenValidated (
    const QWeakObjectPointer& connptr, quint64 id, const QByteArray& token, const UserRecord& user)
{
    // make sure the connection is still in business
    Connection* connection = static_cast<Connection*>(connptr.data());
    if (connection == 0 || !connection->isOpen()) {
        return;
    }

    // create and map the session
    Session* session = new Session(_app, connection, id, token, user);
    _sessions[id] = session;

    // listen for destruction in order to unmap
    connect(session, SIGNAL(destroyed(QObject*)), SLOT(unmapSession(QObject*)));
}
