//
// $Id$

#include <iostream>

#include <signal.h>

#include <QByteArray>
#include <QDateTime>
#include <QMetaObject>
#include <QThreadPool>
#include <QTimer>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/ConnectionManager.h"
#include "scene/SceneManager.h"
#include "util/Mailer.h"

using namespace std;

volatile int pendingSignal = 0;

/**
 * Signal handler that simply sets a variable that the main loop checks periodically.
 */
void signalHandler (int signal)
{
    pendingSignal = signal;
}

/**
 * Returns a string to use in the log entry for a message of the given type.
 */
const char* typeString (QtMsgType type)
{
    switch (type) {
        case QtDebugMsg: return " INFO: ";
        case QtWarningMsg: return " WARNING: ";
        case QtCriticalMsg: return " CRITICAL: ";
        case QtFatalMsg: return " FATAL: ";
        default: return " ???: ";
    }
}

/**
 * Debug message handler that prepends the log time and sends the messages to the main event
 * loop (for thread safety).
 */
void logHandler (QtMsgType type, const char* msg)
{
    static QMetaMethod method = ServerApp::staticMetaObject.method(
        ServerApp::staticMetaObject.indexOfMethod("log(QByteArray)"));
    QByteArray timestamp = QDateTime::currentDateTime().toString(Qt::ISODate).toAscii();
    QCoreApplication* app = QCoreApplication::instance();
    if (app == 0 || app->thread() == QThread::currentThread()) {
        cout << timestamp.constData() << typeString(type) << msg << endl;
    } else {
        method.invoke(app, Q_ARG(const QByteArray&, timestamp + typeString(type) + msg));
    }
}

ServerApp::ServerApp (int& argc, char** argv, const QString& configFile) :
    QCoreApplication(argc, argv),
    _config(configFile, QSettings::IniFormat, this),
    _clientUrl(_config.value("client_url").toString()),
    _bugReportAddress(_config.value("bug_report_address").toString()),
    _mailHostname(_config.value("mail_hostname").toString()),
    _mailPort(_config.value("mail_port").toUInt()),
    _mailFrom(_config.value("mail_from").toString())
{
    // register the signal handler for ^C/HUP
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // register the log handler
    qInstallMsgHandler(logHandler);

    // start the idle timer
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(idle()));
    timer->start(100);

    // create the database thread
    _databaseThread = new DatabaseThread(this);

    // create the managers
    _connectionManager = new ConnectionManager(this);
    _sceneManager = new SceneManager(this);

    // start the database and scene threads
    _databaseThread->start();
    _sceneManager->startThreads();

    // connect the cleanup signal
    connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));

    // note startup
    qDebug() << "Server initialized.";
}

void ServerApp::sendMail (const QString& to, const QString& subject,
    const QString& body, const Callback& callback)
{
    QThreadPool::globalInstance()->start(new Mailer(
        _mailHostname, _mailPort, _mailFrom, to, subject, body, callback));
}

void ServerApp::idle ()
{
    if (pendingSignal == SIGINT) {
        exit();

    } else if (pendingSignal == SIGQUIT) {
        // TODO: some kind of debug output

        pendingSignal = 0;
    }
}

void ServerApp::cleanup ()
{
    // shut down the scene threads
    _sceneManager->stopThreads();

    // shut down the database thread
    _databaseThread->exit();
    _databaseThread->wait();

    // process events again in case debug messages were enqueued
    processEvents();
}

void ServerApp::log (const QByteArray& msg)
{
    cout << msg.constData() << endl;
}
