//
// $Id$

#include "net/Session.h"
#include "scene/EditSceneDialog.h"

EditSceneDialog::EditSceneDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
}
