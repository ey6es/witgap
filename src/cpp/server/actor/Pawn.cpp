//
// $Id$

#include "Protocol.h"
#include "actor/Pawn.h"

Pawn::Pawn (Scene* scene, QChar character, const QPoint& position) :
    Actor(scene, character.unicode() | REVERSE_FLAG, position)
{
}
