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

    // give the pawn, if any, a chance to process the event
    Pawn* pawn = session()->pawn();
    if (pawn != 0) {
        pawn->keyPressEvent(e);
        if (e->isAccepted()) {
            return;
        }
    }

    // pass up to the superclass
    Window::keyPressEvent(e);
}

void MainWindow::keyReleaseEvent (QKeyEvent* e)
{
    // give the pawn, if any, a chance to process the event
    Pawn* pawn = session()->pawn();
    if (pawn != 0) {
        pawn->keyReleaseEvent(e);
        if (e->isAccepted()) {
            return;
        }
    }

    // pass up to the superclass
    Window::keyReleaseEvent(e);
}
