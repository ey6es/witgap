//
// $Id$

#include "RuntimeConfig.h"
#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/PropertyRepository.h"

RuntimeConfig::RuntimeConfig (ServerApp* app) :
    QObject(app),
    _open(false)
{
    // register as a shared object to allow properties to be transmitted between peers
    app->peerManager()->registerSharedObject(this);

    // register as a persistent object to allow properties to be saved to database
    app->databaseThread()->propertyRepository()->registerPersistentObject(this, "RuntimeConfig");
}

RuntimeConfig::RuntimeConfig (QObject* parent) :
    QObject(parent),
    _open(false)
{
}

void RuntimeConfig::setOpen (bool open)
{
    if (_open != open) {
        emit openChanged(_open = open);
    }
}

void RuntimeConfig::setFlags (Flags flags)
{
    if (_flags != flags) {
        emit flagsChanged(_flags = flags);
    }
}
