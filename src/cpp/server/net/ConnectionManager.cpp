//
// $Id$

#include <QTcpSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"

ConnectionManager::ConnectionManager (ServerApp* app) :
    QTcpServer(app),
    _app(app)
{
    // start listening on the configured port
    QHostAddress address(app->config.value("listen_address").toString());
    quint16 port = app->config.value("listen_port").toInt();
    if (!listen(address, port)) {
        qCritical() << "Failed to open server socket:" << errorString();
        return;
    }

    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));
}

ConnectionManager::~ConnectionManager ()
{
}

void ConnectionManager::connectionEstablished (
    Connection* connection, quint64 sessionId,
    const QByteArray& sessionToken, int width, int height)
{
    // if we already have a session and the tokens match, replace it
    Session* session = _sessions[sessionId];
    if (session != 0 && session->token() == sessionToken) {
        session->setConnection(connection);
        return;
    }

    // otherwise, go to the database to validate the token or generate a new one
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "validateToken",
        Q_ARG(quint64, sessionId), Q_ARG(const QByteArray&, sessionToken),
        Q_ARG(const Callback&, Callback(this,
            "tokenValidated(QWeakObjectPointer,quint64,QByteArray)",
            Q_ARG(const QWeakObjectPointer&, QWeakObjectPointer(connection)))));
}

void ConnectionManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new Connection(_app, socket);
    }
}

void ConnectionManager::tokenValidated (
    const QWeakObjectPointer& connptr, quint64 id, const QByteArray& token)
{
    // make sure the connection is still in business
    Connection* connection = static_cast<Connection*>(connptr.data());
    if (connection == 0) {
        return;
    }

    // create and map the session
    _sessions[id] = new Session(_app, connection, id, token);
}
