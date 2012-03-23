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
     * Checks whether the specified session can edit the scene properties.
     */
    bool canSetProperties (Session* session) const;

    /**
     * Sets the scene properties.
     */
    void setProperties (const QString& name, quint16 scrollWidth, quint16 scrollHeight);

    /**
     * Removes the scene from the database.
     */
    void remove ();

    /**
     * Adds a session to the scene.  The session should already be in the scene thread.
     */
    void addSession (Session* session);

    /**
     * Removes a session from the scene.
     */
    void removeSession (Session* session);

protected:

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;
};

#endif // SCENE
