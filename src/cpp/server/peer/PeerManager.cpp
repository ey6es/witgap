//
// $Id$

#include <QSslSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"

PeerManager::PeerManager (ServerApp* app) :
    QTcpServer(app)
{
    // start listening on the configured port
    QHostAddress address(app->config().value("peer_listen_address").toString());
    quint16 port = app->config().value("peer_listen_port").toInt();
    if (!listen(address, port)) {
        qCritical() << "Failed to open peer server socket:" << errorString();
        return;
    }
}

void PeerManager::incomingConnection (int socketDescriptor)
{
    QSslSocket* socket = new QSslSocket(this);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        new PeerConnection(_app, socket);

    } else {
        qWarning() << "Invalid socket descriptor." << socketDescriptor;
        delete socket;
    }
}
