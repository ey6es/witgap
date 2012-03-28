//
// $Id$

#ifndef PAWN
#define PAWN

#include "actor/Actor.h"

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
};

#endif // PAWN
