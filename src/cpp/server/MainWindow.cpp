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
#include "scene/Legend.h"
#include "scene/SceneView.h"
#include "scene/Scene.h"
#include "scene/Zone.h"
#include "ui/Border.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("MainWindow", __VA_ARGS__)

MainWindow::MainWindow (Session* parent) :
    Window(parent, 0, true)
{
    updateTitle();
    setLayout(new BorderLayout());

    addChild(_sceneView = new SceneView(parent), BorderLayout::Center);
    addChild(_legend = new Legend(parent), BorderLayout::East);
}

void MainWindow::updateTitle ()
{
    Session* session = this->session();
    QString name = session->user().name;
    if (!session->loggedOn()) {
        name = tr("%1 (guest)").arg(name);
    }
    QString location;
    if (session->instance() != 0) {
        QString scene;
        if (session->scene() != 0) {
            scene = tr(", %1").arg(session->scene()->record().name);
        }
        location = tr(" in %1 %2%3").arg(session->instance()->record().name,
            QString::number(getInstanceOffset(session->instance()->info().id)), scene);
    }
    setBorder(new TitledBorder(tr("{ %1%2 }").arg(name, location)));
}

void MainWindow::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Session* session = this->session();
    if (modifiers == Qt::NoModifier) {
        int key = e->key();
        switch (key) {
            case Qt::Key_Alt:
            case Qt::Key_Control:
            case Qt::Key_Escape:
                new CommandMenu(session, (key == Qt::Key_Escape) ? -1 : key);
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
