//
// $Id$

#include <iostream>

#include <QtDebug>

#include "ServerApp.h"

using namespace std;

/**
 * Witgap server entry point.
 */
int main (int argc, char** argv)
{
    // we expect a single argument: the path to the server configuration file
    if (argc != 2) {
        cout << "Usage: witgap-server inifile" << endl;
        return 0;
    }
    int result = ServerApp(argc, argv, argv[1]).exec();
    qDebug() << "Server shutdown.";
    return result;
}
