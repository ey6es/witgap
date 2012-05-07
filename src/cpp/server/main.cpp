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
    try {
        int result = ServerApp(argc, argv).exec();
        qDebug() << "Server shutdown.";
        return result;

    } catch (QString error) {
        cerr << error.toStdString();
        return 1;
    }
}
