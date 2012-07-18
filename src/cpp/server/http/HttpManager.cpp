//
// $Id$

#include <QTcpSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "http/HttpConnection.h"
#include "http/HttpManager.h"

HttpManager::HttpManager (ServerApp* app) :
    QTcpServer(app),
    _app(app)
{
    // start listening on the configured port
    QHostAddress address(app->config().value("http_listen_address").toString());
    quint16 port = app->config().value("http_listen_port").toInt() +
        app->args().value("port_offset").toInt();
    if (!listen(address, port)) {
        qCritical() << "Failed to open HTTP server socket:" << errorString();
        return;
    }

    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));
}

void HttpManager::handleRequest (HttpConnection* connection)
{
    connection->respond("404 Not Found", "Resource not found.");
}

void HttpManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new HttpConnection(_app, socket);
    }
}
