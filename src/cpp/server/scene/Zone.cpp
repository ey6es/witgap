//
// $Id$

#include <QMetaObject>
#include <QTimer>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/Zone.h"

Zone::Zone (ServerApp* app, const ZoneRecord& record) :
    CallableObject(app->sceneManager()),
    _app(app),
    _record(record)
{
}

void Zone::createInstance (quint64 sessionId, const Callback& callback)
{
    // get a unique instance id from the lead node
    _app->peerManager()->invokeLead(_app->peerManager(),
        "reserveInstanceId(QString,quint64,Callback)",
        Q_ARG(const QString&, _app->peerManager()->record().name),
        Q_ARG(quint64, createInstanceId(_record.id, 1)), Q_ARG(const Callback&, Callback(_this,
            "continueCreatingInstance(quint64,Callback,quint64)", Q_ARG(quint64, sessionId),
            Q_ARG(const Callback&, callback))));
}

void Zone::updated (const ZoneRecord& record)
{
    _record = record;
    foreach (Instance* instance, _instances) {
        QMetaObject::invokeMethod(instance, "updated", Q_ARG(const ZoneRecord&, record));
    }
}

void Zone::deleted ()
{
    foreach (Instance* instance, _instances) {
        QMetaObject::invokeMethod(instance, "deleted");
    }
}

void Zone::continueCreatingInstance (
    quint64 sessionId, const Callback& callback, quint64 instanceId)
{
    Instance* instance = new Instance(this, instanceId);
    _instances.insert(getInstanceOffset(instanceId), instance);
    callback.invoke(Q_ARG(quint64, instanceId));
}

Instance::Instance (Zone* zone, quint64 id) :
    _zone(zone),
    _record(zone->record())
{
    // add info on all peers
    InstanceInfo info = { id, zone->app()->peerManager()->record().name,
        zone->record().maxPopulation };
    _info = info;
    zone->app()->peerManager()->invoke(zone->app()->peerManager(), "instanceAdded(InstanceInfo)",
        Q_ARG(const InstanceInfo&, info));

    moveToThread(zone->app()->sceneManager()->nextThread());
}

bool Instance::canEdit (Session* session) const
{
    return session->admin();
}

void Instance::setProperties (const QString& name, quint16 maxPopulation, quint32 defaultSceneId)
{
    // update the record
    ZoneRecord record = _record;
    record.name = name;
    record.maxPopulation = maxPopulation;
    record.defaultSceneId = defaultSceneId;

    // update in database
    QMetaObject::invokeMethod(_zone->app()->databaseThread()->sceneRepository(), "updateZone",
        Q_ARG(const ZoneRecord&, record), Q_ARG(const Callback&, Callback(
            _zone->app()->sceneManager(), "broadcastZoneUpdated(ZoneRecord)",
            Q_ARG(const ZoneRecord&, record))));
}

void Instance::remove ()
{
    // remove from database
    QMetaObject::invokeMethod(_zone->app()->databaseThread()->sceneRepository(), "deleteZone",
        Q_ARG(quint32, _record.id), Q_ARG(const Callback&, Callback(
            _zone->app()->sceneManager(), "broadcastZoneDeleted(quint32)",
            Q_ARG(quint32, _record.id))));
}

/** The time for which we hold a reserved place. */
static const int PlaceReservationTimeout = 5000;

void Instance::reservePlace (quint64 sessionId, const Callback& callback)
{
    if (_info.open <= 0) {
        callback.invoke(Q_ARG(bool, false));
        return;
    }
    QTimer* timer = new QTimer(this);
    timer->setProperty("sessionId", sessionId);
    timer->start(PlaceReservationTimeout);
    connect(timer, SIGNAL(timeout()), SLOT(clearPlaceReservation()));
    _placeReservationTimers.insert(sessionId, timer);
    _info.open--;
    _zone->app()->peerManager()->invoke(_zone->app()->peerManager(),
        "instanceUpdated(InstanceInfo)", Q_ARG(const InstanceInfo&, _info));
    callback.invoke(Q_ARG(bool, true));
}

void Instance::cancelPlaceReservation (quint64 sessionId)
{
    QTimer* timer = _placeReservationTimers.take(sessionId);
    if (timer == 0) {
        return; // already cancelled
    }
    timer->deleteLater();
    _info.open++;
    _zone->app()->peerManager()->invoke(_zone->app()->peerManager(),
        "instanceUpdated(InstanceInfo)", Q_ARG(const InstanceInfo&, _info));
}

void Instance::addSession (Session* session)
{
    QTimer* timer = _placeReservationTimers.take(session->record().id);
    if (timer != 0) {
        delete timer;
    }
    session->setParent(this);
}

void Instance::removeSession (Session* session)
{
    session->setParent(0);
    _info.open++;
    _zone->app()->peerManager()->invoke(_zone->app()->peerManager(),
        "instanceUpdated(InstanceInfo)", Q_ARG(const InstanceInfo&, _info));
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

void Instance::updated (const ZoneRecord& record)
{
    // adjust the number of open spaces if necessary
    int maxDelta = (int)record.maxPopulation - _record.maxPopulation;
    if (maxDelta != 0) {
        _info.open += maxDelta;
        _zone->app()->peerManager()->invoke(_zone->app()->peerManager(),
            "instanceUpdated(InstanceInfo)", Q_ARG(const InstanceInfo&, _info));
    }
    emit recordChanged(_record = record);
}

void Instance::deleted ()
{
    // TODO
}

void Instance::clearPlaceReservation ()
{
    cancelPlaceReservation(sender()->property("sessionId").toULongLong());
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
