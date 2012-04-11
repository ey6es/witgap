//
// $Id$

#include <QMetaObject>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/Session.h"
#include "scene/GoToSceneDialog.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/ScrollingList.h"
#include "ui/TextField.h"
#include "util/Callback.h"

// translate through the session
#define tr(...) session()->translate("GoToSceneDialog", __VA_ARGS__)

GoToSceneDialog::GoToSceneDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* ncont = BoxLayout::createHStretchBox(1);
    addChild(ncont);
    ncont->addChild(new Label(tr("Name:")), BoxLayout::Fixed);
    ncont->addChild(_name = new TextField(20, new Document("", 255)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateSelection()));

    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, _showAll = new CheckBox(tr("Show All"))));
    connect(_showAll, SIGNAL(pressed()), SLOT(updateList()));
    _showAll->setVisible(parent->admin());

    addChild(_list = new ScrollingList());
    connect(_list, SIGNAL(selectionChanged()), SLOT(updateGo()));

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_go = new Button(tr("Go")), SIGNAL(pressed()), SLOT(go()));
    _go->setEnabled(false);
    _go->connect(_name, SIGNAL(enterPressed()), SLOT(doPress()));
    _go->connect(_list, SIGNAL(enterPressed()), SLOT(doPress()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _go));

    pack();
    center();

    // request the list of scenes
    updateList();
}

void GoToSceneDialog::updateList ()
{
    Session* session = this->session();
    QMetaObject::invokeMethod(session->app()->databaseThread()->sceneRepository(), "findScenes",
        Q_ARG(const QString&, ""), Q_ARG(quint32, _showAll->selected() ? 0 : session->user().id),
        Q_ARG(const Callback&, Callback(this, "setScenes(SceneDescriptorList)")));
}

void GoToSceneDialog::updateSelection ()
{
    QString prefix = _name->text().simplified();
    for (int ii = 0, nn = _scenes.length(); ii < nn; ii++) {
        if (_scenes.at(ii).name.toLower().startsWith(prefix, Qt::CaseInsensitive)) {
            _list->setSelectedIndex(ii);
            return;
        }
    }
    _list->setSelectedIndex(-1);
}

void GoToSceneDialog::updateGo ()
{
    _go->setEnabled(_list->selectedIndex() != -1);
}

void GoToSceneDialog::go ()
{
    session()->moveToScene(_scenes.at(_list->selectedIndex()).id);
    deleteLater();
}

void GoToSceneDialog::setScenes (const SceneDescriptorList& scenes)
{
    _scenes = scenes;

    QStringList names;
    foreach (const SceneDescriptor& desc, scenes) {
        names.append(desc.name);
    }
    _list->setValues(names);

    updateSelection();
}
