//
// $Id$

#ifndef DATABASE_THREAD
#define DATABASE_THREAD

#include <QThread>

class ServerApp;
class SessionRepository;
class UserRepository;

/**
 * Performs database queries in a separate thread.
 */
class DatabaseThread : public QThread
{
    Q_OBJECT

public:

    /**
     * Initializes the manager.
     */
    DatabaseThread (ServerApp* app);

    /**
     * Returns a reference to the session repository.
     */
    SessionRepository* sessionRepository () const { return _sessionRepository; }

    /**
     * Returns a reference to the user repository.
     */
    UserRepository* userRepository () const { return _userRepository; }

protected:

    /**
     * Connects to the database and processes requests.
     */
    virtual void run ();

    /** The server application. */
    ServerApp* _app;

    /** The connection type. */
    QString _type;

    /** The host to connect to. */
    QString _hostname;

    /** The port to connect on. */
    int _port;

    /** The database name. */
    QString _databaseName;

    /** The username. */
    QString _username;

    /** The user's password. */
    QString _password;

    /** The options to use when connecting. */
    QString _connectOptions;

    /** The session repository. */
    SessionRepository* _sessionRepository;

    /** The user repository. */
    UserRepository* _userRepository;
};

#endif // DATABASE_THREAD
