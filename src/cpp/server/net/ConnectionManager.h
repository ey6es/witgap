//
// $Id$

#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <QTcpServer>

class ServerApp;

/**
 * Listens for TCP connections.
 */
class ConnectionManager : public QTcpServer
{
    Q_OBJECT

public:

    /**
     * Initializes the manager.
     */
    ConnectionManager (ServerApp* app);

    /**
     * Destroys the manager.
     */
    ~ConnectionManager ();

public slots:

    /**
     * Accepts any incoming connections.
     */
    void acceptConnections ();

protected:

    /** The server application. */
    ServerApp* _app;
};

#endif // CONNECTION_MANAGER
