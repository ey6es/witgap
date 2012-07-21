//
// $Id$

#include <QTcpSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "http/HttpConnection.h"
#include "http/HttpManager.h"

void HttpSubrequestHandler::registerSubhandler (const QString& name, HttpRequestHandler* handler)
{
    _subhandlers.insert(name, handler);
}

bool HttpSubrequestHandler::handleRequest (
    HttpConnection* connection, const QString& name, const QString& path)
{
    QString subpath = path;
    if (subpath.startsWith('/')) {
        subpath.remove(0, 1);
    }
    QString subname;
    int idx = subpath.indexOf('/');
    if (idx == -1) {
        subname = subpath;
        subpath = "";
    } else {
        subname = subpath.left(idx);
        subpath = subpath.mid(idx + 1);
    }
    HttpRequestHandler* handler = _subhandlers.value(subname);
    if (handler == 0 || !handler->handleRequest(connection, subname, subpath)) {
        connection->respond("404 Not Found", "Resource not found.");
    }
    return true;
}

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
    _baseUrl = "http://" + _app->peerManager()->record().externalHostname + ":" +
        QString::number(port);

    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));
}

void HttpManager::acceptConnections ()
{
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new HttpConnection(_app, socket);
    }
}
