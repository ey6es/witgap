//
// $Id$

#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <QHash>
#include <QTcpServer>

#include "util/Callback.h"

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
     * Called by a connection when it has received the protocol header.
     */
    void connectionEstablished (
        Connection* connection, quint64 sessionId,
        const QByteArray& sessionToken, int width, int height);

protected slots:

    /**
     * Accepts any incoming connections.
     */
    void acceptConnections ();

    /**
     * Unmaps a destroyed session.
     */
    void unmapSession (QObject* object);

protected:

    /**
     * Callback for validated tokens.
     */
    Q_INVOKABLE void tokenValidated (
        const QWeakObjectPointer& connptr, quint64 id, const QByteArray& token);

    /** The server application. */
    ServerApp* _app;

    /** The set of active sessions, mapped by session id. */
    QHash<quint64, Session*> _sessions;
};

#endif // CONNECTION_MANAGER
