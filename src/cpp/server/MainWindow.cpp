//
// $Id$

#include <QKeyEvent>
#include <QTranslator>
#include <QtDebug>

#include "CommandMenu.h"
#include "MainWindow.h"
#include "actor/Pawn.h"
#include "chat/ChatWindow.h"
#include "net/Session.h"
#include "scene/SceneView.h"
#include "ui/Border.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("MainWindow", __VA_ARGS__)

MainWindow::MainWindow (Session* parent) :
    Window(parent, 0, true)
{
    setBorder(new TitledBorder(tr("witgap")));
    setLayout(new BorderLayout());

    addChild(_sceneView = new SceneView(parent), BorderLayout::Center);
}

void MainWindow::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Session* session = this->session();
    if (modifiers == Qt::NoModifier) {
        switch (e->key()) {
            case Qt::Key_Alt:
            case Qt::Key_Control:
            case Qt::Key_Escape:
                new CommandMenu(session);
                return;

            case Qt::Key_Enter:
            case Qt::Key_Return:
                session->chatEntryWindow()->setVisible(true);
                return;
        }
    }

    // give the pawn, if any, a chance to process the event
    Pawn* pawn = session->pawn();
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
