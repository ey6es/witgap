//
// $Id$

#include <signal.h>

#include <QTimer>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/ConnectionManager.h"
#include "scene/SceneManager.h"

volatile int pendingSignal = 0;

void signalHandler (int signal)
{
    pendingSignal = signal;
}

ServerApp::ServerApp (int& argc, char** argv, const QString& configFile) :
    QCoreApplication(argc, argv),
    _config(configFile, QSettings::IniFormat, this)
{
    // register the signal handler for ^C/HUP
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // start the idle timer
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(idle()));
    timer->start(100);

    // create the database thread
    _databaseThread = new DatabaseThread(this);

    // create the managers
    _connectionManager = new ConnectionManager(this);
    _sceneManager = new SceneManager(this);

    // start the database thread
    _databaseThread->start();

    // connect the cleanup signal
    connect(this, SIGNAL(aboutToQuit()), SLOT(cleanup()));
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
    // shut down the database thread
    _databaseThread->exit();
    _databaseThread->wait();
}
