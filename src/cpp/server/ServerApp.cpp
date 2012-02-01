//
// $Id$

#include <signal.h>

#include <QTimer>

#include "ServerApp.h"
#include "net/ConnectionManager.h"

using namespace std;

volatile int pendingSignal = 0;

void signalHandler (int signal)
{
    pendingSignal = signal;
}

ServerApp::ServerApp (int& argc, char** argv, const QString& configFile) :
    QCoreApplication(argc, argv),
    config(configFile, QSettings::IniFormat, this)
{
    // register the signal handler for ^C/HUP
    signal(SIGINT, signalHandler);
    signal(SIGHUP, signalHandler);

    // start the idle timer
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(idle()));
    timer->start(100);

    // create the connection manager, policy server
    _connectionManager = new ConnectionManager(this);
}

ServerApp::~ServerApp ()
{
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
