//
// $Id$

#include <QMetaObject>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/Zone.h"

Zone::Zone (ServerApp* app, const ZoneRecord& record) :
    QObject(app->sceneManager()),
    _app(app),
    _record(record)
{
}

void Zone::reserveInstancePlace (const Callback& callback)
{
    Instance* instance = new Instance(this);
    instance->moveToThread(_app->sceneManager()->nextThread());
    callback.invoke(Q_ARG(QObject*, instance));
}

Instance::Instance (Zone* zone) :
    _zone(zone)
{
    // add info on all peers
    InstanceInfo info = { 0, zone->app()->peerManager()->record().name };
    _info = info;
    zone->app()->peerManager()->invoke(zone->app()->peerManager(), "instanceAdded(InstanceInfo)",
        Q_ARG(const InstanceInfo&, info));
}

void Instance::resolveScene (quint32 id, const Callback& callback)
{
    // see if it's loaded already
    Scene* scene = _scenes.value(id);
    if (scene != 0) {
        callback.invoke(Q_ARG(QObject*, scene));
        return;
    }

    // add the callback to the penders list
    QList<Callback>& callbacks = _scenePenders[id];
    callbacks.append(callback);
    if (callbacks.size() > 1) {
        return; // database request already pending
    }

    // fetch the scene from the database
    QMetaObject::invokeMethod(_zone->app()->databaseThread()->sceneRepository(), "loadScene",
        Q_ARG(quint32, id), Q_ARG(const Callback&,
            Callback(_this, "sceneMaybeLoaded(quint32,SceneRecord)", Q_ARG(quint32, id))));
}

void Instance::sceneMaybeLoaded (quint32 id, const SceneRecord& record)
{
    // create the scene if it resolved
    Scene* scene = 0;
    if (record.id != 0) {
        _scenes.insert(id, scene = new Scene(_zone->app(), record));
    }

    // remove and notify the penders
    QList<Callback> callbacks = _scenePenders.take(id);
    foreach (const Callback& callback, callbacks) {
        callback.invoke(Q_ARG(QObject*, scene));
    }
}

void Instance::shutdown ()
{
    // remove info on all peers
    _zone->app()->peerManager()->invoke(_zone->app()->peerManager(), "instanceRemoved(quint64)",
        Q_ARG(quint64, _info.id));
}
