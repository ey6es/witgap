//
// $Id$

#ifndef SESSION
#define SESSION

#include <QObject>

class Connection;
class ServerApp;

/**
 * Handles a single user session.
 */
class Session : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the session.
     */
    Session (ServerApp* app, Connection* connection, quint64 id, QByteArray token);

    /**
     * Destroys the session.
     */
    ~Session ();

    /**
     * Returns the session token.
     */
    const QByteArray& token () const { return _token; }

    /**
     * Returns a pointer to the connection, or zero if unconnected.
     */
    Connection* connection () const { return _connection; }

    /**
     * Replaces the session connection.
     */
    void setConnection (Connection* connection);

protected slots:

    /**
     * Indicates that the connection has been destroyed.
     */
    void connectionDestroyed ();

protected:

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    Connection* _connection;

    /** The session id. */
    quint64 _id;

    /** The session token. */
    QByteArray _token;
};

#endif // SESSION
