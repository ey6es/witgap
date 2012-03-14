//
// $Id$

#include <QKeyEvent>

#include "CommandMenu.h"

#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Label.h"
#include "ui/Layout.h"

CommandMenu::CommandMenu (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new TableLayout(1));

    addChild(new Label(QIntVector::createHighlighted(
        parent->loggedOn() ? tr("&Logoff") : tr("&Logon"))));

    pack();
    center();

    requestFocus();
}

void CommandMenu::keyPressEvent (QKeyEvent* e)
{
    switch (e->key()) {
        case Qt::Key_L: {
            Session* sess = session();
            if (sess->loggedOn()) {
                sess->showLogoffDialog();
            } else {
                sess->showLogonDialog();
            }
            deleteLater();
            break;
        }

        default:
            Window::keyPressEvent(e);
            break;
    }
}

void CommandMenu::keyReleaseEvent (QKeyEvent* e)
{
    if (e->key() == Qt::Key_Control) {
        deleteLater();

    } else {
        Window::keyReleaseEvent(e);
    }
}
