//
// $Id$

#include <QTranslator>

#include "admin/AdminMenu.h"
#include "admin/EditUserDialog.h"
#include "net/Session.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("AdminMenu", __VA_ARGS__)

AdminMenu::AdminMenu (Session* parent, int deleteOnReleaseKey) :
    Menu(parent, deleteOnReleaseKey)
{
    addButton(tr("Edit &User..."), &EditUserDialog::staticMetaObject, Q_ARG(Session*, parent));

    pack();
    center();
}
