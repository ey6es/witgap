//
// $Id$

#ifndef ACTOR
#define ACTOR

#include <QObject>
#include <QPoint>

class Scene;

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
    Actor (Scene* scene, int character, const QPoint& position, const QString& label);

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
    int character () const { return _character; }

    /**
     * Sets the actor's position.
     */
    void setPosition (const QPoint& position);

    /**
     * Returns a reference to the actor's position.
     */
    const QPoint& position () const { return _position; }

    /**
     * Sets the actor's label.
     */
    void setLabel (const QString& label);

    /**
     * Returns a reference to the actor's label.
     */
    const QString& label () const { return _label; }

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

    /** The actor's character. */
    int _character;

    /** The actor's position. */
    QPoint _position;

    /** The actor's label, if any. */
    QString _label;
};

#endif // ACTOR
