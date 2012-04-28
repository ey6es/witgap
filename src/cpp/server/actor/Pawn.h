//
// $Id$

#ifndef PAWN
#define PAWN

#include "actor/Actor.h"

class QKeyEvent;

class Session;

/**
 * An active participant in the scene.
 */
class Pawn : public Actor
{
    Q_OBJECT

public:

    /**
     * Creates a new pawn.
     */
    Pawn (Scene* scene, Session* session, const QPoint& position);

    /**
     * Sets the cursor status of the pawn.
     */
    void setCursor (bool cursor);

    /**
     * Returns the cursor status of the pawn.
     */
    bool cursor () const { return _cursor; }

    /**
     * Handles a key press event from the owning session's main window.
     */
    void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event from the owning session's main window.
     */
    void keyReleaseEvent (QKeyEvent* e);

    /**
     * Notifies the actor that the location under it has changed in the scene record.
     */
    virtual void sceneChangedUnderneath (int character);

public slots:

    /**
     * Toggles the pawn's cursor status.
     */
    void toggleCursor () { setCursor(!_cursor); }

protected slots:

    /**
     * Updates the pawn's character.
     */
    void updateCharacter ();

protected:

    /** The session controlling the pawn. */
    Session* _session;

    /** Whether or not the pawn is in cursor mode. */
    bool _cursor;
};

#endif // PAWN
