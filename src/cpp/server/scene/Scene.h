//
// $Id$

#ifndef SCENE
#define SCENE

#include <QObject>

#include "db/SceneRepository.h"

class ServerApp;
class Session;

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

    /**
     * Returns a reference to the scene record.
     */
    const SceneRecord& record () const { return _record; }

    /**
     * Checks whether the specified session can edit the scene.
     */
    bool canEdit (Session* session) const;

    /**
     * Updates the scene metadata.
     */
    void update (const QString& name, quint16 scrollWidth, quint16 scrollHeight);

    /**
     * Removes the scene from the database.
     */
    void remove ();

protected:

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;
};

#endif // SCENE
