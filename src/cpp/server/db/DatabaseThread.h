//
// $Id$

#ifndef DATABASE_THREAD
#define DATABASE_THREAD

#include <QThread>

class ServerApp;

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
     * Destroys the manager.
     */
    ~DatabaseThread ();

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
};

#endif // DATABASE_THREAD
