//
// $Id$

#ifndef SERVER_APP
#define SERVER_APP

#include <QCoreApplication>
#include <QHash>
#include <QList>
#include <QSettings>
#include <QVariant>
#include <QVector>

class QByteArray;
class QTimer;
class QTranslator;

class ArgumentDescriptorList;
class Callback;
class ConnectionManager;
class DatabaseThread;
class PeerManager;
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
    ServerApp (int& argc, char** argv);

    /**
     * Returns the processed command line arguments.
     */
    const QVariantHash& args () const { return _args; }

    /**
     * Returns the application configuration.
     */
    const QSettings& config () const { return _config; }

    /**
     * Returns the client URL.
     */
    const QString& clientUrl () const { return _clientUrl; }

    /**
     * Returns the address to which bug reports should be sent.
     */
    const QString& bugReportAddress () const { return _bugReportAddress; }

    /**
     * Given a locale code, returns the best supported locale.
     */
    QString translationLocale (const QString& locale) const;

    /**
     * Returns a reference to the map from supported locales to translators.
     */
    const QHash<QString, QTranslator*>& translators () const { return _translators; }

    /**
     * Returns a pointer to the connection manager.
     */
    ConnectionManager* connectionManager () const { return _connectionManager; }

    /**
     * Returns a pointer to the peer manager.
     */
    PeerManager* peerManager () const { return _peerManager; }

    /**
     * Returns a pointer to the scene manager.
     */
    SceneManager* sceneManager () const { return _sceneManager; }

    /**
     * Returns a pointer to the database thread.
     */
    DatabaseThread* databaseThread () const { return _databaseThread; }

    /**
     * Attempts to send an email.  The provided callback will receive an empty string on success or
     * an error message on failure.  This function is thread-safe.
     */
    void sendMail (const QString& to, const QString& subject,
        const QString& body, const Callback& callback);

    /**
     * (Re)schedules a reboot.
     */
    Q_INVOKABLE void scheduleReboot (quint32 minutes, const QString& message);

    /**
     * Cancels any scheduled reboot.
     */
    Q_INVOKABLE void cancelReboot ();

protected slots:

    /**
     * Handles idle processing.
     */
    void idle ();

    /**
     * Updates the reboot state.
     */
    void updateReboot ();

    /**
     * Called just before exiting.
     */
    void cleanup ();

protected:

    /**
     * Prints a log message to the standard output.
     */
    Q_INVOKABLE void log (const QByteArray& msg);

    /** The processed command line arguments. */
    QVariantHash _args;

    /** The application configuration. */
    QSettings _config;

    /** The client URL. */
    QString _clientUrl;

    /** The email address to which bug reports are sent. */
    QString _bugReportAddress;

    /** The email hostname. */
    QString _mailHostname;

    /** The email port. */
    quint16 _mailPort;

    /** The email from address. */
    QString _mailFrom;

    /** Translators for supported locales. */
    QHash<QString, QTranslator*> _translators;

    /** The connection manager. */
    ConnectionManager* _connectionManager;

    /** The peer manager. */
    PeerManager* _peerManager;

    /** The scene manager. */
    SceneManager* _sceneManager;

    /** The database thread. */
    DatabaseThread* _databaseThread;

    /** The reboot timer. */
    QTimer* _rebootTimer;

    /** The current index in the reboot countdown. */
    int _rebootCountdownIdx;

    /** The custom reboot message, if any. */
    QString _rebootMessage;
};

/**
 * Describes a command line argument.
 */
class ArgumentDescriptor
{
public:

    /** The argument name. */
    QString name;

    /** The argument description. */
    QString description;

    /** The default value. */
    QVariant defvalue;

    /** The parameter types. */
    QVector<QVariant::Type> parameterTypes;
};

/**
 * A list of argument descriptors.
 */
class ArgumentDescriptorList : public QList<ArgumentDescriptor>
{
public:

    /**
     * Adds a description to the list.
     */
    void append (
        const QString& name, const QString& description, const QVariant& defvalue = QVariant(),
        QVariant::Type t1 = QVariant::Invalid, QVariant::Type t2 = QVariant::Invalid,
        QVariant::Type t3 = QVariant::Invalid, QVariant::Type t4 = QVariant::Invalid,
        QVariant::Type t5 = QVariant::Invalid);
};

#endif // SERVER_APP
