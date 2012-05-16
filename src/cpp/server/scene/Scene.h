//
// $Id$

#ifndef SCENE
#define SCENE

#include <QList>
#include <QObject>

#include "actor/Actor.h"
#include "chat/ChatWindow.h"
#include "db/SceneRepository.h"

class Pawn;
class SceneBlock;
class SceneView;
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
     * Represents a directly renderable block of the scene.
     */
    class Block : public QIntVector
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
        Block ();

        /**
         * Sets the number of non-empty locations in the block.
         */
        void setFilled (int filled) { _filled = filled; };

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
    const QHash<QPoint, Block>& blocks () const { return _blocks; }

    /**
     * Checks whether the specified session can edit the scene.
     */
    bool canEdit (Session* session) const;

    /**
     * Sets the scene properties.
     */
    void setProperties (const QString& name, quint16 scrollWidth, quint16 scrollHeight);

    /**
     * Sets a location in the scene.
     */
    void set (const QPoint& pos, int character);

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
     * Adds the specified actor's visual representation to the scene contents.  This is done when
     * the actor is created, and just after the actor is moved.
     */
    void addSpatial (Actor* actor);

    /**
     * Removes the specified actor's visual representation from the scene contents.  This is done
     * when the actor is destroyed, and just before the actor is moved.
     */
    void removeSpatial (Actor* actor) { removeSpatial(actor, actor->character()); };

    /**
     * Removes the specified actor's visual representation from the scene contents.  This is done
     * when the actor is destroyed, and just before the actor is moved.
     */
    void removeSpatial (Actor* actor, int character);

    /**
     * Notes that an actor's character has changed.
     *
     * @param ocharacter the previous character.
     */
    void characterChanged (Actor* actor, int ochar);

    /**
     * Adds a scene view to the map.  This is done when the session is added, and just after the
     * view is moved/resized.
     */
    void addSpatial (SceneView* view);

    /**
     * Removes a scene view from the map.  This is done when the session is removed, and just
     * before the view is moved/resized.
     */
    void removeSpatial (SceneView* view);

    /**
     * Sends a message to all views intersecting the specified location.
     */
    void say (const QPoint& pos, const QString& speaker,
        const QString& message, ChatWindow::SpeakMode mode);

signals:

    /**
     * Fired when the scene properties have been modified.
     */
    void propertiesChanged ();

public slots:

    /**
     * Flushes the scene to the database.
     */
    void flush ();

protected:

    /**
     * Sets a character in the scene blocks.
     */
    void setInBlocks (const QPoint& pos, int character);

    /**
     * Dirties a single location in all views that can see it.
     */
    void dirty (const QPoint& pos);

    /** A list of scene views. */
    typedef QList<SceneView*> SceneViewList;

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;

    /** The current set of scene blocks. */
    QHash<QPoint, Block> _blocks;

    /** Maps locations to linked lists of actors. */
    QHash<QPoint, Actor*> _actors;

    /** The sessions in the scene. */
    QList<Session*> _sessions;

    /** Maps block locations to lists of intersecting views. */
    QHash<QPoint, SceneViewList> _views;

    /** The size of the view hash space blocks as a power of two. */
    static const int LgViewBlockSize = 7;
};

#endif // SCENE
