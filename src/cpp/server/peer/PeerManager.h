//
// $Id$

#ifndef PEER_MANAGER
#define PEER_MANAGER

#include <QTcpServer>

class ServerApp;

/**
 * Manages peer bits.
 */
class PeerManager : public QTcpServer
{
    Q_OBJECT

public:

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
};

#endif // PEER_MANAGER
