//
// $Id$

#ifndef ACTOR
#define ACTOR

#include <QObject>
#include <QPair>
#include <QPoint>
#include <QSharedData>
#include <QSharedPointer>

class Scene;

/** Pairs a visible character with its label. */
typedef QPair<int, QString> CharacterLabelPair;

/** A pointer to a label. */
typedef CharacterLabelPair* LabelPointer;

/**
 * The actor's shared data.
 */
class ActorData : public QSharedData
{
public:

    /** The actor's character. */
    int character;
    
    /** The label pointer. */
    QSharedPointer<CharacterLabelPair> label;
    
    /** The actor's collision flags. */
    int collisionFlags;
    
    /** The actor's collision mask. */
    int collisionMask;
    
    /** The actor's spawn mask. */
    int spawnMask;
};

/**
 * An active participant in the scene.
 */
class Actor : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new actor.
     */
    Actor (Scene* scene, int character, const QString& label, const QPoint& position,
        int collisionFlags, int collisionMask, int spawnMask);

    /**
     * Destroys the actor.
     */
    virtual ~Actor ();

    /**
     * Sets the next actor in the linked list.
     */
    void setNext (Actor* next) { _next = next; }

    /**
     * Returns the next actor in the linked list.
     */
    Actor* next () const { return _next; }

    /**
     * Sets the actor's character.
     */
    void setCharacter (int character);

    /**
     * Returns the actor's character.
     */
    int character () const { return _data->character; }

    /**
     * Sets the actor's label.
     */
    void setLabel (const QString& label);

    /**
     * Returns the actor's label.
     */
    QString label () const { return _data->label.isNull() ? QString() : _data->label->second; }

    /**
     * Returns a pointer to the actor's label, or zero for none.
     */
    LabelPointer labelPointer () const { return _data->label.data(); }

    /**
     * Sets the actor's collision flags.
     */
    void setCollisionFlags (int flags);
    
    /**
     * Returns the actor's collision flags.
     */
    int collisionFlags () const { return _data->collisionFlags; }
    
    /**
     * Sets the actor's collision mask.
     */
    void setCollisionMask (int mask) { _data->collisionMask = mask; }
    
    /**
     * Returns the actor's collision mask.
     */
    int collisionMask () const { return _data->collisionMask; }
    
    /**
     * Sets the actor's spawn mask.
     */
    void setSpawnMask (int mask) { _data->spawnMask = mask; }

    /**
     * Returns the actor's spawn mask.
     */
    int spawnMask () const { return _data->spawnMask; }

    /**
     * Attempts to move to the specified position, returning true if successful.
     */
    bool move (const QPoint& position);

    /**
     * Sets the actor's position.
     */
    void setPosition (const QPoint& position);

    /**
     * Returns a reference to the actor's position.
     */
    const QPoint& position () const { return _position; }

    /**
     * Notifies the actor that the location under it has changed in the scene record.
     */
    virtual void sceneChangedUnderneath (int character) { };

signals:

    /**
     * Fired when the actor's position has changed.
     *
     * @param opos the actor's original position.
     */
    void positionChanged (const QPoint& opos);

protected:

    /** The scene containing the actor. */
    Scene* _scene;

    /** The next actor in the list, if any. */
    Actor* _next;

    /** The actor's position. */
    QPoint _position;
    
    /** The actor's shared data. */
    QSharedDataPointer<ActorData> _data;
};

#endif // ACTOR
