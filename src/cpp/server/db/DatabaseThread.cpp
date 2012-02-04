//
// $Id$

#include <QtDebug>
#include <QSqlDatabase>
#include <QSqlError>

#include "ServerApp.h"
#include "db/DatabaseThread.h"

DatabaseThread::DatabaseThread (ServerApp* app) :
    QThread(app),
    _app(app),
    _type(app->config.value("database_type").toString()),
    _hostname(app->config.value("database_hostname").toString()),
    _port(app->config.value("database_port").toInt()),
    _databaseName(app->config.value("database_name").toString()),
    _username(app->config.value("database_username").toString()),
    _password(app->config.value("database_password").toString()),
    _connectOptions(app->config.value("database_connect_options").toString())
{
}

DatabaseThread::~DatabaseThread ()
{
}

void DatabaseThread::run ()
{
    // connect to the configured database
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(_type, "db");
        db.setHostName(_hostname);
        db.setPort(_port);
        db.setDatabaseName(_databaseName);
        db.setUserName(_username);
        db.setPassword(_password);
        db.setConnectOptions(_connectOptions);
        if (db.open()) {
            // enter event loop
            exec();
        } else {
            qCritical() << "Failed to connect to database:" << db.lastError();
        }
    }
    QSqlDatabase::removeDatabase("db");
}