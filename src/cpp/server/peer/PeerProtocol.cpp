//
// $Id$

#include <QDataStream>
#include <QtDebug>

#include "ServerApp.h"
#include "peer/Peer.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"
#include "util/Callback.h"

void PeerMessage::handle (Peer* peer) const
{
    qWarning() << "Message not supported for downstream.";
}

void PeerMessage::handle (PeerConnection* connection) const
{
    qWarning() << "Message not supported for upstream.";
}

void CloseMessage::handle (Peer* peer) const
{
    peer->deleteLater();
}

void CloseMessage::handle (PeerConnection* connection) const
{
    connection->deleteLater();
}

void ExecuteMessage::handle (PeerConnection* connection) const
{
    const PeerAction* paction = static_cast<const PeerAction*>(action.constData());
    paction->execute(connection->app());
}

void ExecuteLeadMessage::handle (PeerConnection* connection) const
{
    connection->app()->peerManager()->executeLead(action);
}

void ExecuteSessionMessage::handle (PeerConnection* connection) const
{
    connection->app()->peerManager()->executeSession(name, action);
}

void RequestMessage::handle (PeerConnection* connection) const
{
    const PeerRequest* prequest = static_cast<const PeerRequest*>(request.constData());
    prequest->handle(connection->app(), Callback(connection, "sendResponse(quint32,QVariantList)",
        Q_ARG(quint32, id)).setCollate());
}

void RequestLeadMessage::handle (PeerConnection* connection) const
{
    connection->app()->peerManager()->requestLead(request, Callback(connection,
        "sendResponse(quint32,QVariantList)", Q_ARG(quint32, id)).setCollate());
}

void RequestSessionMessage::handle (PeerConnection* connection) const
{
    connection->app()->peerManager()->requestSession(name, request, Callback(connection,
        "sendResponse(quint32,QVariantList)", Q_ARG(quint32, id)).setCollate());
}

void ResponseMessage::handle (Peer* peer) const
{
    peer->handleResponse(requestId, args);
}
