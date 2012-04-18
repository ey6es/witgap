//
// $Id$

#include <QKeyEvent>
#include <QTranslator>

#include "admin/AdminMenu.h"
#include "admin/EditUserDialog.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Label.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("AdminMenu", __VA_ARGS__)

AdminMenu::AdminMenu (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new TableLayout(1));

    addChild(new Label(QIntVector::createHighlighted(tr("Edit &User"))));

    pack();
    center();

    requestFocus();
}

void AdminMenu::keyPressEvent (QKeyEvent* e)
{
    Session* session = this->session();
    switch (e->key()) {
        case Qt::Key_U:
            new EditUserDialog(session);
            deleteLater();
            break;

        default:
            Window::keyPressEvent(e);
            break;
    }
}

void AdminMenu::keyReleaseEvent (QKeyEvent* e)
{
    if (e->key() == Qt::Key_Alt) {
        deleteLater();

    } else {
        Window::keyReleaseEvent(e);
    }
}
