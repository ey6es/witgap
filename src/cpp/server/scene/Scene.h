//
// $Id$

#ifndef SCENE
#define SCENE

#include <QObject>

#include "db/SceneRepository.h"

class Actor;
class Pawn;
class ServerApp;
class Session;

/**
 * The in-memory representation of a scene.
 */
class Scene : public QObject
{
    Q_OBJECT

public:

    /** The size of the scene blocks as a power of two. */
    static const int LgBlockSize = 5;

    /**
     * Creates a new scene.
     */
    Scene (ServerApp* app, const SceneRecord& record);

    /**
     * Returns a reference to the scene record.
     */
    const SceneRecord& record () const { return _record; }

    /**
     * Returns a reference to the scene contents.
     */
    const QHash<QPoint, QIntVector>& contents () const { return _contents; }

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
     *
     * @return a pointer to the pawn created for the session, or 0 for none.
     */
    Pawn* addSession (Session* session);

    /**
     * Removes a session from the scene.
     */
    void removeSession (Session* session);

    /**
     * Adds the specified actor's visual representation to the scene contents.
     */
    void addToContents (Actor* actor);

    /**
     * Removes the specified actor's visual representation from the scene contents.
     */
    void removeFromContents (Actor* actor);

protected:

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;

    /** The current scene contents. */
    QHash<QPoint, QIntVector> _contents;
};

#endif // SCENE
