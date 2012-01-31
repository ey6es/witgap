//
// $Id$

#include <QTcpSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"

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

void ConnectionManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new Connection(_app, socket);
    }
}
