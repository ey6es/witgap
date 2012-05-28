//
// $Id$

#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <QHash>
#include <QTcpServer>

#include <openssl/pem.h>

#include "net/Connection.h"
#include "peer/PeerManager.h"
#include "util/Callback.h"

class ServerApp;
class Session;
class SessionRecord;
class TranslationKey;
class UserRecord;

/**
 * Listens for TCP connections.
 */
class ConnectionManager : public QTcpServer, public SharedObject
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
     * Returns a pointer to the private RSA key.
     */
    RSA* rsa () const { return _rsa; }

    /**
     * Called by a connection when it has received the protocol header.
     */
    void connectionEstablished (Connection* connection);

    /**
     * Broadcasts a message to all online users.
     */
    Q_INVOKABLE void broadcast (const QString& speaker, const QString& message);

    /**
     * Broadcasts a message to all online users.
     */
    Q_INVOKABLE void broadcast (const TranslationKey& key);

    /**
     * Attempts to send a tell to the named recipient.  The callback will receive the
     * tell result: a bool indicating whether the recipient could be reached.
     */
    Q_INVOKABLE void tell (
        const QString& speaker, const QString& message,
        const QString& recipient, const Callback& callback);

    /**
     * Notifies the manager that a session's name has changed (because of logon/logoff).
     */
    Q_INVOKABLE void sessionNameChanged (const QString& oldName, const QString& newName);

    /**
     * Notifies the manager that a session has been closed.
     */
    Q_INVOKABLE void sessionClosed (quint64 id, const QString& name);

protected slots:

    /**
     * Accepts any incoming connections.
     */
    void acceptConnections ();

protected:

    /**
     * Callback for connection installation.
     */
    Q_INVOKABLE void connectionMaybeSet (const SharedConnectionPointer& connptr, bool success);

    /**
     * Callback for validated tokens.
     */
    Q_INVOKABLE void tokenValidated (
        const SharedConnectionPointer& connptr, const SessionRecord& record,
        const UserRecord& user);

    /** The server application. */
    ServerApp* _app;

    /** The private RSA key. */
    RSA* _rsa;

    /** The set of active sessions, mapped by session id. */
    QHash<quint64, Session*> _sessions;

    /** Sessions mapped by name. */
    QHash<QString, Session*> _names;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
};

#endif // CONNECTION_MANAGER
