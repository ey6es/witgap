//
// $Id$

#include <QTranslator>

#include "admin/AdminMenu.h"
#include "admin/EditUserDialog.h"
#include "admin/GenerateInvitesDialog.h"
#include "admin/RuntimeConfigDialog.h"
#include "net/Session.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("AdminMenu", __VA_ARGS__)

AdminMenu::AdminMenu (Session* parent, int deleteOnReleaseKey) :
    Menu(parent, deleteOnReleaseKey)
{
    addButton(tr("Edit &User..."), &EditUserDialog::staticMetaObject, Q_ARG(Session*, parent));
    addButton(tr("Runtime &Config..."), &RuntimeConfigDialog::staticMetaObject,
        Q_ARG(Session*, parent));
    addButton(tr("Generate &Invites..."), &GenerateInvitesDialog::staticMetaObject,
        Q_ARG(Session*, parent));

    pack();
    center();
}
