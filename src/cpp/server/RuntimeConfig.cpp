//
// $Id$

#include "RuntimeConfig.h"
#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/PropertyRepository.h"

RuntimeConfig::RuntimeConfig (ServerApp* app) :
    QObject(app),
    _logonPolicy(AdminsOnly)
{
    // register as a shared object to allow properties to be transmitted between peers
    app->peerManager()->registerSharedObject(this);

    // register as a persistent object to allow properties to be saved to database
    app->databaseThread()->propertyRepository()->registerPersistentObject(this, "RuntimeConfig");
}

RuntimeConfig::RuntimeConfig (QObject* parent) :
    QObject(parent),
    _logonPolicy(AdminsOnly)
{
}

void RuntimeConfig::setLogonPolicy (LogonPolicy policy)
{
    if (_logonPolicy != policy) {
        emit logonPolicyChanged(_logonPolicy = policy);
    }
}
