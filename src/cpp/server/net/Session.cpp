//
// $Id$

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"

Session::Session (ServerApp* app, Connection* connection, quint64 id, QByteArray token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(connection),
    _id(id),
    _token(token)
{
    // send the session info back to the connection and activate it
    connection->setSession(id, token);
    connection->activate();
}

Session::~Session ()
{
}

void Session::setConnection (Connection* connection)
{
    _connection->deactivate();
    _connection = connection;
    _connection->activate();
}
