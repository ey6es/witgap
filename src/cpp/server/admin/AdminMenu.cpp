//
// $Id$

#include "admin/AdminMenu.h"

#include "net/Session.h"
#include "ui/Border.h"

AdminMenu::AdminMenu (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
}
