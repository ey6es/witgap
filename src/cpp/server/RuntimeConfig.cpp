//
// $Id$

#include "RuntimeConfig.h"
#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/PropertyRepository.h"

RuntimeConfig::RuntimeConfig (ServerApp* app) :
    _logonPolicy(AdminsOnly),
    _introZone(0),
    _introScene(0)
{
    // register as a shared object to allow properties to be transmitted between peers
    app->peerManager()->registerSharedObject(this);

    // register as a persistent object to allow properties to be saved to database
    app->databaseThread()->propertyRepository()->registerPersistentObject(this, "RuntimeConfig");
}

RuntimeConfig::RuntimeConfig (QObject* parent) :
    QObject(parent),
    _logonPolicy(AdminsOnly),
    _introZone(0),
    _introScene(0)
{
}

void RuntimeConfig::setLogonPolicy (LogonPolicy policy)
{
    if (_logonPolicy != policy) {
        emit logonPolicyChanged(_logonPolicy = policy);
    }
}

void RuntimeConfig::setIntroZone (quint32 id)
{
    if (_introZone != id) {
        emit introZoneChanged(_introZone = id);
    }
}

void RuntimeConfig::setIntroScene (quint32 id)
{
    if (_introScene != id) {
        emit introSceneChanged(_introScene = id);
    }
}
