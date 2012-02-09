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
    Session (ServerApp* app, Connection* connection, QByteArray token);

    /**
     * Destroys the session.
     */
    ~Session ();

    /**
     * Returns the session token.
     */
    const QByteArray& token () const { return _token; }

    /**
     * Replaces the session connection.
     */
    void setConnection (Connection* connection);

protected:

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    Connection* _connection;

    /** The session token. */
    QByteArray _token;
};

#endif // SESSION
