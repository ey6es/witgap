//
// $Id$

#ifndef HTTP_MANAGER
#define HTTP_MANAGER

#include <QByteArray>
#include <QHash>
#include <QTcpServer>

class HttpConnection;
class HttpRequestHandler;
class ServerApp;

/**
 * Interface for HTTP request handlers.
 */
class HttpRequestHandler
{
public:

    /**
     * Handles an HTTP request.
     */
    virtual bool handleRequest (
        HttpConnection* connection, const QString& name, const QString& path) = 0;
};

/**
 * Handles requests by forwarding them to subhandlers.
 */
class HttpSubrequestHandler : public HttpRequestHandler
{
public:

    /**
     * Registers a subhandler with the given name.
     */
    void registerSubhandler (const QString& name, HttpRequestHandler* handler);

    /**
     * Handles an HTTP request.
     */
    virtual bool handleRequest (
        HttpConnection* connection, const QString& name, const QString& path);

protected:

    /** Subhandlers mapped by name. */
    QHash<QString, HttpRequestHandler*> _subhandlers;
};

/**
 * Handles HTTP connections.
 */
class HttpManager : public QTcpServer, public HttpSubrequestHandler
{
   Q_OBJECT

public:

    /**
     * Initializes the manager.
     */
    HttpManager (ServerApp* app);

    /**
     * Returns a reference to the server's base URL.
     */
    const QString& baseUrl () const { return _baseUrl; }

protected slots:

    /**
     * Accepts all pending connections.
     */
    void acceptConnections ();

protected:

    /** The server application. */
    ServerApp* _app;

    /** The base URL for the server. */
    QString _baseUrl;
};

#endif // HTTP_MANAGER
