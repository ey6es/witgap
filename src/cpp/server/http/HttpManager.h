//
// $Id$

#ifndef HTTP_MANAGER
#define HTTP_MANAGER

#include <QTcpServer>

class HttpConnection;
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

    /**
     * Handles a request received from a connection.
     */
    void handleRequest (HttpConnection* connection);

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
