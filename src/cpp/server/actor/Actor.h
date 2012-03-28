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
    Actor (Scene* scene, int character, const QPoint& position);

    /**
     * Destroys the actor.
     */
    virtual ~Actor ();

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

protected:

    /** The scene containing the actor. */
    Scene* _scene;

    /** The actor's character. */
    int _character;

    /** The actor's position. */
    QPoint _position;
};

#endif // ACTOR
