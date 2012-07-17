//
// $Id$

#ifndef HTTP_CONNECTION
#define HTTP_CONNECTION

#include <QHash>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QObject>

class QTcpSocket;

class ServerApp;

/**
 * Handles a single HTTP connection.
 */
class HttpConnection : public QObject
{
   Q_OBJECT

public:

    /** Header hash. */
    typedef QHash<QByteArray, QByteArray> Headers;

    /**
     * Initializes the connection.
     */
    HttpConnection (ServerApp* app, QTcpSocket* socket);

    /**
     * Destroys the connection.
     */
    virtual ~HttpConnection ();

    /**
     * Sends a response and closes the connection.
     */
    void respond (const char* code, const QByteArray& content = QByteArray(),
        const char* contentType = "text/plain; charset=ISO-8859-1",
        const Headers& headers = Headers());

protected slots:

    /**
     * Reads the request line.
     */
    void readRequest ();

    /**
     * Reads the headers.
     */
    void readHeaders ();

    /**
     * Reads the content.
     */
    void readContent ();

protected:

    /**
     * Handles the request, once we've received it in full.
     */
    void handleRequest ();

    /** The server application. */
    ServerApp* _app;

    /** The underlying socket. */
    QTcpSocket* _socket;

    /** The stored address. */
    QHostAddress _address;

    /** The requested operation. */
    QNetworkAccessManager::Operation _operation;

    /** The requested resource. */
    QByteArray _resource;

    /** The request headers. */
    Headers _requestHeaders;

    /** The last request header processed (used for continuations). */
    QByteArray _lastRequestHeader;

    /** The content of the request. */
    QByteArray _requestContent;
};

#endif // HTTP_CONNECTION
