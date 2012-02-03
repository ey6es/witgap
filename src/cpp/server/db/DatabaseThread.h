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
};

#endif // DATABASE_THREAD
