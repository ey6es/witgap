//
// $Id$

#include <QDataStream>
#include <QtDebug>

#include "peer/Peer.h"
#include "peer/PeerConnection.h"
#include "peer/PeerManager.h"
#include "peer/PeerProtocol.h"

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
