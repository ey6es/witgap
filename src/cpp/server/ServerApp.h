//
// $Id$

#ifndef SERVER_APP
#define SERVER_APP

#include <QCoreApplication>
#include <QSettings>

class QByteArray;

class ConnectionManager;
class DatabaseThread;
class SceneManager;

/**
 * The Witgap server application.
 */
class ServerApp : public QCoreApplication
{
    Q_OBJECT

public:

    /**
     * Initializes the application.
     *
     * @param configFile the path to the application configuration file.
     */
    ServerApp (int& argc, char** argv, const QString& configFile);

    /**
     * Returns the application configuration.
     */
    const QSettings& config () const { return _config; }

    /**
     * Returns a pointer to the connection manager.
     */
    ConnectionManager* connectionManager () const { return _connectionManager; }

    /**
     * Returns a pointer to the scene manager.
     */
    SceneManager* sceneManager () const { return _sceneManager; }

    /**
     * Returns a pointer to the database thread.
     */
    DatabaseThread* databaseThread () const { return _databaseThread; }

protected slots:

    /**
     * Handles idle processing.
     */
    void idle ();

    /**
     * Called just before exiting.
     */
    void cleanup ();

protected:

    /**
     * Prints a log message to the standard output.
     */
    Q_INVOKABLE void log (const QByteArray& msg);

    /** The application configuration. */
    QSettings _config;

    /** The connection manager. */
    ConnectionManager* _connectionManager;

    /** The scene manager. */
    SceneManager* _sceneManager;

    /** The database thread. */
    DatabaseThread* _databaseThread;
};

#endif // SERVER_APP
