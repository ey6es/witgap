//
// $Id$

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "ui/Window.h"

Session::Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(0),
    _id(id),
    _token(token)
{
    // send the session info back to the connection and activate it
    connection->setSession(id, token);
    setConnection(connection);

    Window* window = new Window(this);
    window->setBounds(QRect(10, 10, 20, 20));
    window->setBackground(0x21);
}

void Session::setConnection (Connection* connection)
{
    if (_connection != 0) {
        _connection->disconnect(this);
        _connection->deactivate(tr("Logged in elsewhere."));
    }
    _connection = connection;
    connect(_connection, SIGNAL(windowClosed()), SLOT(deleteLater()));
    connect(_connection, SIGNAL(destroyed()), SLOT(clearConnection()));
    _connection->activate();
}

void Session::clearConnection ()
{
    _connection = 0;
}
