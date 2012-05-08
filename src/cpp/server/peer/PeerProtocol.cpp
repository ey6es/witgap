//
// $Id$

#include <QDataStream>
#include <QMetaType>
#include <QtDebug>

#include "peer/Peer.h"
#include "peer/PeerConnection.h"
#include "peer/PeerProtocol.h"

void PeerMessage::handle (Peer* peer) const
{
    qWarning() << "Message not supported for downstream." << QMetaType::typeName(type());
}

void PeerMessage::handle (PeerConnection* connection) const
{
    qWarning() << "Message not supported for upstream." << QMetaType::typeName(type());
}

template <class T> int registerMessageType (const char* typeName)
{
    int type = qRegisterMetaType<T>(typeName);
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

int CloseMessage::type () const
{
    return closeMessageType;
}

void CloseMessage::handle (Peer* peer) const
{
    peer->deleteLater();
}

void CloseMessage::handle (PeerConnection* connection) const
{
    connection->deleteLater();
}
