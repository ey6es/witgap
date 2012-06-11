//
// $Id$

#ifndef CONNECTION_MANAGER
#define CONNECTION_MANAGER

#include <QHash>
#include <QPair>
#include <QTcpServer>

#include <openssl/pem.h>

#include <GeoIP.h>

#include "net/Connection.h"
#include "peer/PeerManager.h"
#include "util/Callback.h"

class QTimer;

class ServerApp;
class Session;
class SessionRecord;
class SessionTransfer;
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
    virtual ~ConnectionManager ();

    /**
     * Returns a pointer to the private RSA key.
     */
    RSA* rsa () const { return _rsa; }

    /**
     * Returns a pointer to the GeoIP database.
     */
    GeoIP* geoIp () const { return _geoIp; }

    /**
     * Called by a connection when it has received the protocol header.
     */
    void connectionEstablished (Connection* connection);

    /**
     * Transfers a session from another peer.  The callback will receive the (QString) host name
     * and (quint16) port number.
     */
    Q_INVOKABLE void transferSession (const SessionTransfer& transfer, const Callback& callback);

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

    /**
     * Called when a pending transfer has expired.
     */
    void clearPendingTransfer ();

protected:

    /** A transfer awaiting connection. */
    typedef QPair<SessionTransfer, QTimer*> PendingTransfer;

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

    /** The GeoIP database. */
    GeoIP* _geoIp;

    /** The set of active sessions, mapped by session id. */
    QHash<quint64, Session*> _sessions;

    /** Sessions mapped by name. */
    QHash<QString, Session*> _names;

    /** Pending session transfers. */
    QHash<quint64, PendingTransfer> _pendingTransfers;

    /** Synchronized pointer for callbacks. */
    CallablePointer _this;
};

#endif // CONNECTION_MANAGER
