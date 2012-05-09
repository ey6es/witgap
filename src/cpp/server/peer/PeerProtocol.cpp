//
// $Id$

#include <QDataStream>
#include <QtDebug>

#include "peer/Peer.h"
#include "peer/PeerConnection.h"
#include "peer/PeerProtocol.h"

void PeerMessage::handle (Peer* peer) const
{
    qWarning() << "Message not supported for downstream.";
}

void PeerMessage::handle (PeerConnection* connection) const
{
    qWarning() << "Message not supported for upstream.";
}

template<class T> int registerMessageType (const char* typeName)
{
    int type = qRegisterMetaType<T>();
    qRegisterMetaTypeStreamOperators<T>(typeName);
    return type;
}

QDataStream& operator<< (QDataStream& out, const CloseMessage& msg)
{
    return out;
}

QDataStream& operator>> (QDataStream& in, CloseMessage& msg)
{
    return in;
}

int closeMessageType = registerMessageType<CloseMessage>("CloseMessage");

void CloseMessage::handle (Peer* peer) const
{
    peer->deleteLater();
}

void CloseMessage::handle (PeerConnection* connection) const
{
    connection->deleteLater();
}


QDataStream& operator<< (QDataStream& out, const ExecuteMessage& msg)
{
    out << msg.action;
    return out;
}

QDataStream& operator>> (QDataStream& in, ExecuteMessage& msg)
{
    in >> msg.action;
    return in;
}

int executeMessageType = registerMessageType<ExecuteMessage>("ExecuteMessage");

void ExecuteMessage::handle (PeerConnection* connection) const
{

}
