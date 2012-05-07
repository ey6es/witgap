//
// $Id$

#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <QTcpServer>

#include "db/PeerRepository.h"

class ArgumentDescriptorList;
class ServerApp;

/**
 * Manages peer bits.
 */
class PeerManager : public QTcpServer
{
    Q_OBJECT

public:

    /**
     * Appends the peer-related command line arguments to the specified list.
     */
    static void appendArguments (ArgumentDescriptorList* args);

    /**
     * Initializes the manager.
     */
    PeerManager (ServerApp* app);

protected:

    /**
     * Handles an incoming connection.
     */
    virtual void incomingConnection (int socketDescriptor);

    /** The server application. */
    ServerApp* _app;

    /** Our own peer record. */
    PeerRecord _record;
};

#endif // PEER_MANAGER
