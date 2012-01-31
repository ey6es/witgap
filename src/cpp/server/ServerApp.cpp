//
// $Id$

#include <signal.h>

#include <QTimer>

#include "ServerApp.h"
#include "net/ConnectionManager.h"

using namespace std;

volatile bool shouldExit = false;

void signalHandler (int signal)
{
    shouldExit = true;
}

ServerApp::ServerApp (int& argc, char** argv, const QString& configFile) :
    QCoreApplication(argc, argv),
    config(configFile, QSettings::IniFormat, this)
{
    // register the signal handler for ^C
    signal(SIGINT, signalHandler);

    // start the idle timer
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(idle()));
    timer->start(100);

    // create the connection manager
    _connectionManager = new ConnectionManager(this);
}

ServerApp::~ServerApp ()
{
}

void ServerApp::idle ()
{
    if (shouldExit) {
        exit();
    }
}
