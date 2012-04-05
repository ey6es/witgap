//
// $Id$

#ifndef SCENE
#define SCENE

#include <QObject>

#include "db/SceneRepository.h"

class Actor;
class Pawn;
class SceneBlock;
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
     * Returns a reference to the scene block map.
     */
    const QHash<QPoint, SceneBlock>& blocks () const { return _blocks; }

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

    /** The current set of scene blocks. */
    QHash<QPoint, SceneBlock> _blocks;

    /** Maps locations to linked lists of actors. */
    QHash<QPoint, Actor*> _actors;
};

/**
 * Represents a directly renderable block of the scene.
 */
class SceneBlock : public QIntVector
{
public:

    /** The width/height of each block as a power of two. */
    static const int LgSize = 5;

    /** The width/height of each block. */
    static const int Size = (1 << LgSize);

    /** The mask for coordinates. */
    static const int Mask = Size - 1;

    /**
     * Creates an empty scene block.
     */
    SceneBlock ();

    /**
     * Returns the number of non-empty locations in the block.
     */
    int filled () const { return _filled; }

    /**
     * Sets the character at the specified position.
     */
    void set (const QPoint& pos, int character);

protected:

    /** The number of non-empty locations in the block. */
    int _filled;
};

#endif // SCENE
