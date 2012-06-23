//
// $Id$

#include <QMetaObject>
#include <QThread>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "scene/SceneManager.h"
#include "scene/Zone.h"

SceneManager::SceneManager (ServerApp* app) :
    CallableObject(app),
    _app(app),
    _lastThreadIdx(0)
{
    // register for remote invocation
    _app->peerManager()->registerSharedObject(this);

    // create the configured number of scene threads
    int nthreads = app->config().value("scene_threads").toInt();
    if (nthreads == -1) {
        nthreads = qMax(1, QThread::idealThreadCount() - 1);
    }
    for (int ii = 0; ii < nthreads; ii++) {
        _threads.append(new QThread(this));
    }
}

Instance* SceneManager::instance (quint64 id) const
{
    Zone* zone = _zones.value(getZoneId(id));
    return (zone == 0) ? 0 : zone->instances().value(getInstanceOffset(id));
}

void SceneManager::startThreads ()
{
    foreach (QThread* thread, _threads) {
        thread->start();
    }
}

void SceneManager::stopThreads ()
{
//    foreach (Scene* scene, _scenes) {
//        QMetaObject::invokeMethod(scene, "flush");
//    }
    foreach (QThread* thread, _threads) {
        thread->exit();
        thread->wait();
    }
}

QThread* SceneManager::nextThread ()
{
    _lastThreadIdx = (_lastThreadIdx + 1) % _threads.size();
    return _threads.at(_lastThreadIdx);
}

void SceneManager::createInstance (quint64 sessionId, quint32 zoneId, const Callback& callback)
{
    resolveZone(zoneId, Callback(_this, "continueCreatingInstance(quint64,Callback,QObject*)",
        Q_ARG(quint64, sessionId), Q_ARG(const Callback&, callback)));
}

void SceneManager::reserveInstancePlace (
    quint64 sessionId, quint64 instanceId, const Callback& callback)
{
    Instance* instance = this->instance(instanceId);
    if (instance != 0) {
        QMetaObject::invokeMethod(instance, "reservePlace", Q_ARG(quint64, sessionId),
            Q_ARG(const Callback&, callback));
    } else {
        callback.invoke(Q_ARG(bool, false));
    }
}

void SceneManager::cancelInstancePlaceReservation (quint64 sessionId, quint64 instanceId)
{
    Instance* instance = this->instance(instanceId);
    if (instance != 0) {
        QMetaObject::invokeMethod(instance, "cancelPlaceReservation", Q_ARG(quint64, sessionId));
    }
}

void SceneManager::resolveZone (quint32 id, const Callback& callback)
{
    // see if it's loaded already
    Zone* zone = _zones.value(id);
    if (zone != 0) {
        callback.invoke(Q_ARG(QObject*, zone));
        return;
    }

    // add the callback to the penders list
    QList<Callback>& callbacks = _zonePenders[id];
    callbacks.append(callback);
    if (callbacks.size() > 1) {
        return; // database request already pending
    }

    // fetch the scene from the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "loadZone",
        Q_ARG(quint32, id), Q_ARG(const Callback&,
            Callback(_this, "zoneMaybeLoaded(quint32,ZoneRecord)", Q_ARG(quint32, id))));
}

void SceneManager::broadcastZoneRecordUpdated (const ZoneRecord& record)
{
    _app->peerManager()->invoke(this, "zoneRecordUpdated(ZoneRecord)",
        Q_ARG(const ZoneRecord&, record));
}

void SceneManager::zoneRecordUpdated (const ZoneRecord& record)
{
    Zone* zone = _zones.value(record.id);
    if (zone != 0) {
        zone->recordUpdated(record);
    }
}

void SceneManager::zoneMaybeLoaded (quint32 id, const ZoneRecord& record)
{
    // create the zone if it resolved
    Zone* zone = 0;
    if (record.id != 0) {
        _zones.insert(id, zone = new Zone(_app, record));
    }

    // remove and notify the penders
    QList<Callback> callbacks = _zonePenders.take(id);
    foreach (const Callback& callback, callbacks) {
        callback.invoke(Q_ARG(QObject*, zone));
    }
}

void SceneManager::continueCreatingInstance (
    quint64 sessionId, const Callback& callback, QObject* zobj)
{
    if (zobj == 0) {
        callback.invoke(Q_ARG(quint64, 0));
        return;
    }
    static_cast<Zone*>(zobj)->createInstance(sessionId, callback);
}
