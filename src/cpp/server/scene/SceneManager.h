//
// $Id$

#ifndef SCENE_MANAGER
#define SCENE_MANAGER

#include <QHash>
#include <QObject>

class Scene;
class ServerApp;

/**
 * Manages the set of loaded scenes.
 */
class SceneManager : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the scene manager.
     */
    SceneManager (ServerApp* app);

    /**
     * Moves a session to the identified scene.
     */
//    Q_INVOKABLE void moveToScene (quint32 sceneId);

protected:

    /** The application object. */
    ServerApp* _app;

    /** Loaded scenes mapped by id. */
    QHash<quint32, Scene*> _scenes;
};

#endif // SCENE_MANAGER
