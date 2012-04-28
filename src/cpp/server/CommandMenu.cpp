//
// $Id$

#include <QTranslator>

#include "CommandMenu.h"
#include "SettingsDialog.h"
#include "actor/Pawn.h"
#include "admin/AdminMenu.h"
#include "net/Session.h"
#include "scene/GoToSceneDialog.h"
#include "scene/Scene.h"
#include "scene/ScenePropertiesDialog.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("CommandMenu", __VA_ARGS__)

CommandMenu::CommandMenu (Session* parent, int deleteOnReleaseKey) :
    Menu(parent, deleteOnReleaseKey)
{
    if (parent->admin()) {
        addButton(tr("&Admin"), &AdminMenu::staticMetaObject,
            Q_ARG(Session*, parent), Q_ARG(int, deleteOnReleaseKey));
    }
    bool loggedOn = parent->loggedOn();
    if (loggedOn) {
        addButton(tr("&Go to Scene"), &GoToSceneDialog::staticMetaObject, Q_ARG(Session*, parent));
        addButton(tr("&New Scene"), parent, SLOT(createScene()));

        Scene* scene = parent->scene();
        if (scene != 0 && scene->canEdit(parent)) {
            addButton(tr("Scene &Properties"), &ScenePropertiesDialog::staticMetaObject,
                Q_ARG(Session*, parent));

            Pawn* pawn = parent->pawn();
            if (pawn != 0) {
                addButton(tr("Toggle &Edit Mode"), pawn, SLOT(toggleCursor()));
            }
        }
    }

    addButton(tr("&Settings"), &SettingsDialog::staticMetaObject, Q_ARG(Session*, parent));
    if (loggedOn) {
        addButton(tr("&Logoff"), parent, SLOT(showLogoffDialog()));
    } else {
        addButton(tr("&Logon"), parent, SLOT(showLogonDialog()));
    }

    pack();
    center();
}
