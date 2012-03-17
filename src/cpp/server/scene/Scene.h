//
// $Id$

#ifndef SCENE
#define SCENE

#include <QObject>

#include "db/SceneRepository.h"

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
    Scene (const SceneRecord& record);

protected:

    /** The scene record. */
    SceneRecord _record;
};

#endif // SCENE
