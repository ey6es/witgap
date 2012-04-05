//
// $Id$

#ifndef PAWN
#define PAWN

#include "actor/Actor.h"

class QKeyEvent;

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
    Pawn (Scene* scene, QChar character, const QPoint& position);

    /**
     * Handles a key press event from the owning session's main window.
     */
    void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event from the owning session's main window.
     */
    void keyReleaseEvent (QKeyEvent* e);
};

#endif // PAWN
