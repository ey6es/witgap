//
// $Id$

#include <QMetaObject>
#include <QThread>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"

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
    foreach (QThread* thread, _threads) {
        thread->exit();
        thread->wait();
    }
}

void SceneManager::resolveScene (quint32 id, const Callback& callback)
{
    // see if it's loaded already
    Scene* scene = _scenes.value(id);
    if (scene != 0) {
        callback.invoke(Q_ARG(Scene*, scene));
        return;
    }

    // add the callback to the penders list
    QList<Callback>& callbacks = _penders[id];
    callbacks.append(callback);
    if (callbacks.size() > 1) {
        return; // database request already pending
    }

    // fetch the scene from the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "loadScene",
        Q_ARG(quint32, id), Q_ARG(const Callback&,
            Callback(_this, "sceneMaybeLoaded(quint32,SceneRecord)", Q_ARG(quint32, id))));
}

void SceneManager::sceneMaybeLoaded (quint32 id, const SceneRecord& record)
{
    // create the scene if it resolved
    Scene* scene = 0;
    if (record.id != 0) {
        scene = new Scene(_app, record);

        // assign to a thread in round-robin fashion
        _lastThreadIdx = (_lastThreadIdx + 1) % _threads.size();
        scene->moveToThread(_threads.at(_lastThreadIdx));
    }

    // remove and notify the penders
    QList<Callback> callbacks = _penders.take(id);
    foreach (const Callback& callback, callbacks) {
        callback.invoke(Q_ARG(QObject*, scene));
    }
}
