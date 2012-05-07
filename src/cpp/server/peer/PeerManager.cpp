//
// $Id$

#include <QSslSocket>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"

void PeerManager::appendArguments (ArgumentDescriptorList* args)
{
    args->append("name", "The peer name.", "local", QVariant::String);
    args->append("region", "The geographic region.", "us-east", QVariant::String);
    args->append("internal_hostname", "The hostname within the region.",
        "localhost", QVariant::String);
    args->append("external_hostname", "The hostname outside the region.",
        "localhost", QVariant::String);
}

PeerManager::PeerManager (ServerApp* app) :
    QTcpServer(app)
{
    // start listening on the configured port
    QHostAddress address(app->config().value("peer_listen_address").toString());
    quint16 port = app->config().value("peer_listen_port").toInt() +
        app->args().value("port_offset").toInt();
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
