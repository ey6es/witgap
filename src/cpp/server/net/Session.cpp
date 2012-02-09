//
// $Id$

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"

Session::Session (ServerApp* app, Connection* connection, QByteArray token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(connection),
    _token(token)
{
    _connection->activate();
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
