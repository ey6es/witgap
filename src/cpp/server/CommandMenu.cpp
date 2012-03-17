//
// $Id$

#include <QKeyEvent>

#include "CommandMenu.h"

#include "admin/AdminMenu.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Label.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) session()->translate("CommandMenu", __VA_ARGS__)

CommandMenu::CommandMenu (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new TableLayout(1));

    if (parent->admin()) {
        addChild(new Label(QIntVector::createHighlighted(tr("&Admin"))));
    }
    if (parent->loggedOn()) {
        addChild(new Label(QIntVector::createHighlighted(tr("&New Scene"))));
        addChild(new Label(QIntVector::createHighlighted(tr("&Logoff"))));
    } else {
        addChild(new Label(QIntVector::createHighlighted(tr("&Logon"))));
    }

    pack();
    center();

    requestFocus();
}

void CommandMenu::keyPressEvent (QKeyEvent* e)
{
    Session* session = this->session();
    switch (e->key()) {
        case Qt::Key_A:
            if (session->admin()) {
                new AdminMenu(session);
                deleteLater();
            }
            break;

        case Qt::Key_L:
            if (session->loggedOn()) {
                session->showLogoffDialog();
            } else {
                session->showLogonDialog();
            }
            deleteLater();
            break;

        case Qt::Key_N:
            if (session->loggedOn()) {
                session->createScene();
                deleteLater();
            }
            break;

        default:
            Window::keyPressEvent(e);
            break;
    }
}

void CommandMenu::keyReleaseEvent (QKeyEvent* e)
{
    if (e->key() == Qt::Key_Alt) {
        deleteLater();

    } else {
        Window::keyReleaseEvent(e);
    }
}
