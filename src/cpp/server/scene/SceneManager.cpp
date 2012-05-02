//
// $Id$

#include <QMetaObject>
#include <QThread>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/Zone.h"

SceneManager::SceneManager (ServerApp* app) :
    CallableObject(app),
    _app(app),
    _lastThreadIdx(0)
{
    // create the configured number of scene threads
    int nthreads = app->config().value("scene_threads").toInt();
    if (nthreads == -1) {
        nthreads = qMax(1, QThread::idealThreadCount() - 1);
    }
    for (int ii = 0; ii < nthreads; ii++) {
        _threads.append(new QThread(this));
    }
}

void SceneManager::startThreads ()
{
    foreach (QThread* thread, _threads) {
        thread->start();
    }
}

void SceneManager::stopThreads ()
{
    foreach (Scene* scene, _scenes) {
        QMetaObject::invokeMethod(scene, "flush");
    }
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

void SceneManager::resolveScene (quint32 id, const Callback& callback)
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
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "loadScene",
        Q_ARG(quint32, id), Q_ARG(const Callback&,
            Callback(_this, "sceneMaybeLoaded(quint32,SceneRecord)", Q_ARG(quint32, id))));
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

void SceneManager::sceneMaybeLoaded (quint32 id, const SceneRecord& record)
{
    // create the scene if it resolved
    Scene* scene = 0;
    if (record.id != 0) {
        _scenes.insert(id, scene = new Scene(_app, record));

        // assign to a thread in round-robin fashion
        scene->moveToThread(nextThread());
        QMetaObject::invokeMethod(scene, "init");
    }

    // remove and notify the penders
    QList<Callback> callbacks = _scenePenders.take(id);
    foreach (const Callback& callback, callbacks) {
        callback.invoke(Q_ARG(QObject*, scene));
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
