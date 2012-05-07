//
// $Id$

#include "ServerApp.h"
#include "peer/Peer.h"
#include "peer/PeerManager.h"

Peer::Peer (ServerApp* app, const PeerRecord& record) :
    QObject(app->peerManager()),
    _app(app),
    _record(record)
{
}
