//
// $Id$

#ifndef SCENE_MANAGER
#define SCENE_MANAGER

#include <QHash>
#include <QList>
#include <QObject>
#include <QVector>

#include "util/Callback.h"

class QThread;

class Scene;
class SceneRecord;
class ServerApp;

/**
 * Manages the set of loaded scenes.
 */
class SceneManager : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Initializes the scene manager.
     */
    SceneManager (ServerApp* app);

    /**
     * Starts the scene threads.
     */
    void startThreads ();

    /**
     * Stops the scene threads.
     */
    void stopThreads ();

    /**
     * Attempts to resolve a scene.  The callback will receive a QObject*, either the resolved
     * scene or 0 if not found.
     */
    Q_INVOKABLE void resolveScene (quint32 id, const Callback& callback);

protected:

    /**
     * Called when a request to load a scene returns.
     */
    Q_INVOKABLE void sceneMaybeLoaded (quint32 id, const SceneRecord& record);

    /** The application object. */
    ServerApp* _app;

    /** Loaded scenes mapped by id. */
    QHash<quint32, Scene*> _scenes;

    /** Callbacks waiting pending scene resolution. */
    QHash<quint32, QList<Callback> > _penders;

    /** The list of scene threads. */
    QVector<QThread*> _threads;

    /** The index of the last thread to which we assigned a scene. */
    int _lastThreadIdx;
};

#endif // SCENE_MANAGER
