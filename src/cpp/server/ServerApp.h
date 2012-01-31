//
// $Id$

#ifndef SERVER_APP
#define SERVER_APP

#include <QCoreApplication>
#include <QSettings>

class ConnectionManager;

/**
 * The Witgap server application.
 */
class ServerApp : public QCoreApplication
{
    Q_OBJECT

public:

    /**
     * Application configuration.
     */
    const QSettings config;

    /**
     * Initializes the application.
     *
     * @param configFile the path to the application configuration file.
     */
    ServerApp (int& argc, char** argv, const QString& configFile);

    /**
     * Destroys the application.
     */
    ~ServerApp ();

    /**
     * Returns a pointer to the session manager.
     */
    ConnectionManager* connectionManager () const { return _connectionManager; }

public slots:

    /**
     * Handles idle processing.
     */
    void idle ();

protected:

    /** The connection manager. */
    ConnectionManager* _connectionManager;
};

#endif // SERVER_APP
