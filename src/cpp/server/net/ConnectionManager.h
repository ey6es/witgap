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
class SessionRecord;
class UserRecord;

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
        Connection* connection, quint64 sessionId, const QByteArray& sessionToken);

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
        const QWeakObjectPointer& connptr, const SessionRecord& record, const UserRecord& user);

    /** The server application. */
    ServerApp* _app;

    /** The set of active sessions, mapped by session id. */
    QHash<quint64, Session*> _sessions;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
};

#endif // CONNECTION_MANAGER
