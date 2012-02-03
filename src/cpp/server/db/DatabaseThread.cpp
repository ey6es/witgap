//
// $Id$

#include <QtDebug>
#include <QSqlDatabase>
#include <QSqlError>

#include "ServerApp.h"
#include "db/DatabaseThread.h"

DatabaseThread::DatabaseThread (ServerApp* app) :
    QThread(app),
    _app(app)
{
}

DatabaseThread::~DatabaseThread ()
{
}

void DatabaseThread::run ()
{
    // connect to the configured database
    const QSettings& config = _app->config;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(
            config.value("database_type").toString(), "db");
        db.setHostName(config.value("database_hostname").toString());
        db.setPort(config.value("database_port").toInt());
        db.setDatabaseName(config.value("database_name").toString());
        db.setUserName(config.value("database_username").toString());
        db.setPassword(config.value("database_password").toString());
        db.setConnectOptions(config.value("database_connect_options").toString());
        if (db.open()) {
            // enter event loop
            exec();
        } else {
            qCritical() << "Failed to connect to database:" << db.lastError();
        }
    }
    QSqlDatabase::removeDatabase("db");
}
