//
// $Id$

#ifndef HTTP_MANAGER
#define HTTP_MANAGER

#include <QTcpServer>

class ServerApp;

/**
 * Handles HTTP connections.
 */
class HttpManager : public QTcpServer
{
   Q_OBJECT

public:

    /**
     * Initializes the manager.
     */
    HttpManager (ServerApp* app);

protected slots:

    /**
     * Accepts all pending connections.
     */
    void acceptConnections ();

protected:

    /** The server application. */
    ServerApp* _app;
};

#endif // HTTP_MANAGER
