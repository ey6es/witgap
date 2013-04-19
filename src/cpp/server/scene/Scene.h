//
// $Id$

#ifndef SCENE
#define SCENE

#include <QList>
#include <QObject>
#include <QPair>

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
     * Template for the various block types.
     */
    template<class T, int E> class GenericBlock : public QVector<T>
    {
    public:
        
        /** The width/height of each block as a power of two. */
        static const int LgSize = 5;

        /** The width/height of each block. */
        static const int Size = (1 << LgSize);

        /** The mask for coordinates. */
        static const int Mask = Size - 1;
        
        /**
         * Creates an empty block.
         */
        GenericBlock () : QVector<T>(Size*Size, E), _filled(0) { }
        
        /**
         * Sets the number of non-empty locations in the block.
         */
        void setFilled (int filled) { _filled = filled; };

        /**
         * Returns the number of non-empty locations in the block.
         */
        int filled () const { return _filled; }
    
        /**
         * Sets the value at the specified position, returning the old value.
         */
        T set (const QPoint& pos, T nvalue)
        {
            T& value = (*this)[(pos.y() & Mask) << LgSize | pos.x() & Mask];
            T ovalue = value;
            value = nvalue;
            _filled += (ovalue == E) - (nvalue == E);
            return ovalue;
        }
        
        /**
         * Sets the bits in the specified value.
         */
        void setFlags (const QPoint& pos, T flags)
        {
            T& value = (*this)[(pos.y() & Mask) << LgSize | pos.x() & Mask];
            if (value == 0 && flags != 0) {
                _filled++;
            }
            value |= flags;
        }
        
        /**
         * Returns the value at the specified position.
         */
        T get (const QPoint& pos) const
        {
            return this->at((pos.y() & Mask) << LgSize | pos.x() & Mask);
        }
        
    protected:

        /** The number of non-empty locations in the block. */
        int _filled;
    };

    /** Represents a directly renderable block of the scene. */
    typedef GenericBlock<int, ' '> Block;

    /** Represents a block of label pointers. */
    typedef GenericBlock<LabelPointer, 0> LabelBlock;

    /** Represents a block of flag pointers. */
    typedef GenericBlock<int, 0> FlagBlock;

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
     * Returns a reference to the label map.
     */
    const QHash<QPoint, LabelBlock>& labels () const { return _labels; }

    /**
     * Returns a reference to the collision flag map.
     */
    const QHash<QPoint, FlagBlock>& collisionFlags () const { return _collisionFlags; }

    /**
     * Returns the collision flags at the specified point.
     */
    int collisionFlags (const QPoint& pos) const;

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
    Pawn* addSession (Session* session, const QVariant& portal);

    /**
     * Removes a session from the scene.
     */
    void removeSession (Session* session);

    /**
     * Notes that the scene record has been updated in the database.
     */
    void updated (const SceneRecord& record);

    /**
     * Notes that the scene has been deleted from the database.
     */
    void deleted ();

    /**
     * Adds the specified actor's visual representation to the scene contents.  This is done when
     * the actor is created, and just after the actor is moved.
     */
    void addSpatial (Actor* actor);

    /**
     * Removes the specified actor's visual representation from the scene contents.  This is done
     * when the actor is destroyed, and just before the actor is moved.
     *
     * @param npos the position to which the actor will be moving, or zero for none.
     */
    void removeSpatial (Actor* actor, const QPoint* npos = 0) {
        removeSpatial(actor, actor->character(), npos); };

    /**
     * Removes the specified actor's visual representation from the scene contents.  This is done
     * when the actor is destroyed, and just before the actor is moved.
     *
     * @param npos the position to which the actor will be moving, or zero for none.
     */
    void removeSpatial (Actor* actor, int character, const QPoint* npos = 0);

    /**
     * Notes that an actor's character or label has changed.
     *
     * @param ocharacter the previous character.
     */
    void characterLabelChanged (Actor* actor, int ochar);

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
     * Attempts to find a path from the given start point to the specified end.  Returns an empty
     * path on failure.
     */
    QVector<QPoint> findPath (
        const QPoint& start, const QPoint& end, int collisionMask, int maxLength) const;

    /**
     * Sends a message to all views intersecting the specified location.
     */
    void say (const QPoint& pos, const QString& speaker,
        const QString& message, ChatWindow::SpeakMode mode);

signals:

    /**
     * Fired when the scene record has been modified.
     */
    void recordChanged (const SceneRecord& record);

public slots:

    /**
     * Flushes the scene to the database.
     */
    void flush ();

protected:
    
    /** A list of scene views. */
    typedef QList<SceneView*> SceneViewList;

    /**
     * Updates the collision flags at the specified position from the actor list.
     */
    void updateCollisionFlags (const QPoint& pos, Actor* actor);

    /**
     * Sets a character in the scene blocks.
     */
    void setInBlocks (const QPoint& pos, int character,
        LabelPointer label = 0, const QPoint* npos = 0);

    /** The application object. */
    ServerApp* _app;

    /** The scene record. */
    SceneRecord _record;

    /** The current set of scene blocks. */
    QHash<QPoint, Block> _blocks;

    /** The label blocks. */
    QHash<QPoint, LabelBlock> _labels;

    /** The collision flag blocks. */
    QHash<QPoint, FlagBlock> _collisionFlags;

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
