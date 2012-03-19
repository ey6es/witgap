//
// $Id$

#include "net/Session.h"
#include "scene/GoToSceneDialog.h"
#include "ui/Border.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) session()->translate("GoToSceneDialog", __VA_ARGS__)

GoToSceneDialog::GoToSceneDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
}
