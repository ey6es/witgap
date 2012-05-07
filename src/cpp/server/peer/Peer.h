//
// $Id$

#ifndef PEER
#define PEER

#include <QObject>

#include "db/PeerRepository.h"

class ServerApp;

/**
 * Handles a single peer.
 */
class Peer : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the peer.
     */
    Peer (ServerApp* app, const PeerRecord& record);

protected:

    /** The server application. */
    ServerApp* _app;

    /** The latest peer record. */
    PeerRecord _record;
};

#endif // PEER
