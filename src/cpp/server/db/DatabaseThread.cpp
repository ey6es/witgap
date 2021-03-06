//
// $Id$

#include <time.h>

#include <QtDebug>
#include <QSqlDatabase>
#include <QSqlError>

#include "ServerApp.h"
#include "db/ActorRepository.h"
#include "db/DatabaseThread.h"
#include "db/PeerRepository.h"
#include "db/PropertyRepository.h"
#include "db/SceneRepository.h"
#include "db/UserRepository.h"
#include "util/General.h"

DatabaseThread::DatabaseThread (ServerApp* app) :
    QThread(app),
    _app(app),
    _type(app->config().value("database_type").toString()),
    _hostname(app->config().value("database_hostname").toString()),
    _port(app->config().value("database_port").toInt()),
    _databaseName(app->config().value("database_name").toString()),
    _username(app->config().value("database_username").toString()),
    _password(app->config().value("database_password").toString()),
    _connectOptions(app->config().value("database_connect_options").toString()),
    _actorRepository(new ActorRepository()),
    _peerRepository(new PeerRepository()),
    _propertyRepository(new PropertyRepository(app)),
    _sceneRepository(new SceneRepository()),
    _userRepository(new UserRepository(app))
{
    // move the repositories to this thread
    _actorRepository->moveToThread(this);
    _peerRepository->moveToThread(this);
    _propertyRepository->moveToThread(this);
    _sceneRepository->moveToThread(this);
    _userRepository->moveToThread(this);
}

void DatabaseThread::run ()
{
    // seed the random number generator for this thread
    qsrand(currentTimeMillis());

    // get our pointer to the runtime config
    _runtimeConfig = _app->runtimeConfig();

    // connect to the configured database
    QString connectionName;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(_type);
        connectionName = db.connectionName();
        db.setHostName(_hostname);
        db.setPort(_port);
        db.setDatabaseName(_databaseName);
        db.setUserName(_username);
        db.setPassword(_password);
        db.setConnectOptions(_connectOptions);
        if (db.open()) {
            // initialize repositories
            _actorRepository->init();
            _peerRepository->init();
            _propertyRepository->init();
            _sceneRepository->init();
            _userRepository->init();

            // enter event loop
            exec();

        } else {
            qCritical() << "Failed to connect to database:" << db.lastError();
        }
    }
    QSqlDatabase::removeDatabase(connectionName);
}
