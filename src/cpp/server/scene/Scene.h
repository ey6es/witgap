//
// $Id$

#ifndef SCENE
#define SCENE

#include <QObject>

#include "db/SceneRepository.h"

class ServerApp;

/**
 * The in-memory representation of a scene.
 */
class Scene : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new scene.
     */
    Scene (ServerApp* app, const SceneRecord& record);

protected:

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;
};

#endif // SCENE
