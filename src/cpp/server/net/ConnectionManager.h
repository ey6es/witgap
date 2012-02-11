//
// $Id$

#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <QHash>
#include <QTcpServer>

class Connection;
class ServerApp;
class Session;

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

    /**
     * Called by a connection when it has received the protocol header.
     */
    void connectionEstablished (
        Connection* connection, quint64 sessionId,
        const QByteArray& sessionToken, int width, int height);

    /**
     * Callback for validated tokens.
     */
    Q_INVOKABLE void tokenValidated (QObject* connection, quint64 id, const QByteArray& token);

public slots:

    /**
     * Accepts any incoming connections.
     */
    void acceptConnections ();

protected:

    /** The server application. */
    ServerApp* _app;

    /** The set of active sessions, mapped by session id. */
    QHash<quint64, Session*> _sessions;
};

#endif // CONNECTION_MANAGER
