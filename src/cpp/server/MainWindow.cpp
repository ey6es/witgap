//
// $Id$

#include <QKeyEvent>

#include "CommandMenu.h"
#include "MainWindow.h"
#include "actor/Pawn.h"
#include "net/Session.h"
#include "scene/SceneView.h"
#include "ui/Border.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) session()->translate("MainWindow", __VA_ARGS__)

MainWindow::MainWindow (Session* parent) :
    Window(parent, 0, true)
{
    setBorder(new TitledBorder(tr("witgap")));
    setLayout(new BorderLayout());

    addChild(_sceneView = new SceneView(), BorderLayout::Center);
}

void MainWindow::keyPressEvent (QKeyEvent* e)
{
    int key = e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if ((key == Qt::Key_Alt || key == Qt::Key_Escape) && modifiers == Qt::NoModifier) {
        new CommandMenu(session());
        return;
    }

    // all printable characters and various control characters are forwarded to the pawn (if any)
    Pawn* pawn = session()->pawn();
    if (pawn != 0) {
    }

    Window::keyPressEvent(e);
}
