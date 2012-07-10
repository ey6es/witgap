//
// $Id$

#include <QTranslator>

#include "CommandMenu.h"
#include "SettingsDialog.h"
#include "actor/Pawn.h"
#include "admin/AdminMenu.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/Zone.h"
#include "scene/ScenePropertiesDialog.h"
#include "scene/ZonePropertiesDialog.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("CommandMenu", __VA_ARGS__)

CommandMenu::CommandMenu (Session* parent, int deleteOnReleaseKey) :
    Menu(parent, deleteOnReleaseKey)
{
    if (parent->admin()) {
        addButton(tr("&Admin >"), &AdminMenu::staticMetaObject,
            Q_ARG(Session*, parent), Q_ARG(int, deleteOnReleaseKey));
    }
    bool loggedOn = parent->loggedOn();
    if (loggedOn) {
        addButton(tr("&New >"), this, SLOT(createNewMenu()));
        addButton(tr("&Edit >"), this, SLOT(createEditMenu()));

        Scene* scene = parent->scene();
        if (scene != 0 && scene->canEdit(parent)) {
            Pawn* pawn = parent->pawn();
            if (pawn != 0) {
                addButton(tr("Toggle &Cursor Mode"), pawn, SLOT(toggleCursor()));
            }
        }
    }

    addButton(tr("&Settings..."), &SettingsDialog::staticMetaObject, Q_ARG(Session*, parent));
    addButton(tr("&Tools >"), this, SLOT(createToolsMenu()));
    if (loggedOn) {
        addButton(tr("&Logoff"), parent, SLOT(showLogoffDialog()));
    } else {
        addButton(tr("&Logon..."), parent, SLOT(showLogonDialog()));
    }

    pack();
    center();
}

void CommandMenu::createNewMenu ()
{
    Session* session = this->session();
    Menu* menu = new Menu(session, _deleteOnReleaseKey);

    menu->addButton(tr("&Zone"), session, SLOT(createZone()));
    menu->addButton(tr("&Scene"), session, SLOT(createScene()));

    menu->pack();
    menu->center();
}

void CommandMenu::createEditMenu ()
{
    Session* session = this->session();
    Menu* menu = new Menu(session, _deleteOnReleaseKey);

    Instance* instance = session->instance();
    if (instance != 0 && instance->canEdit(session)) {
        menu->addButton(tr("&Zone Properties..."), &ZonePropertiesDialog::staticMetaObject,
            Q_ARG(Session*, session));
    }

    Scene* scene = session->scene();
    if (scene != 0 && scene->canEdit(session)) {
        menu->addButton(tr("&Scene Properties..."), &ScenePropertiesDialog::staticMetaObject,
            Q_ARG(Session*, session));
    }

    menu->pack();
    menu->center();
}

void CommandMenu::createToolsMenu ()
{
    Session* session = this->session();
    Menu* menu = new Menu(session, _deleteOnReleaseKey);

    menu->addButton(tr("Go to &Zone"), session, SLOT(showGoToZoneDialog()));
    menu->addButton(tr("Go to &Scene"), session, SLOT(showGoToSceneDialog()));

    menu->pack();
    menu->center();
}
