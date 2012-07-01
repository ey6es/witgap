//
// $Id$

#include <QMetaObject>
#include <QTranslator>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/IdChooserDialog.h"
#include "ui/Layout.h"
#include "ui/ScrollingList.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("IdChooserDialog", __VA_ARGS__)

IdChooserDialog::IdChooserDialog (Session* parent, quint32 id, bool allowZero) :
    Window(parent, parent->highestWindowLayer(), true, true),
    _initialId(id),
    _allowZero(allowZero)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* ncont = BoxLayout::createHStretchBox(1);
    addChild(ncont);
    ncont->addChild(new Label(tr("Name:")), BoxLayout::Fixed);
    ncont->addChild(_name = new TextField(20, new Document("", 255)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateSelection()));

    addChild(_list = new ScrollingList());
    if (!allowZero) {
        connect(_list, SIGNAL(selectionChanged()), SLOT(updateOk()));
    }

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    _ok = new Button(tr("OK"));
    _ok->setEnabled(false);
    _ok->connect(_name, SIGNAL(enterPressed()), SLOT(doPress()));
    _ok->connect(_list, SIGNAL(enterPressed()), SLOT(doPress()));
    connect(_ok, SIGNAL(pressed()), SLOT(emitIdSelected()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _ok));

    pack();
    center();
}

void IdChooserDialog::updateSelection ()
{
    QString prefix = _name->text().simplified();
    for (int ii = 0, nn = _list->values().length(); ii < nn; ii++) {
        if (_list->values().at(ii).toLower().startsWith(prefix, Qt::CaseInsensitive)) {
            _list->setSelectedIndex(ii);
            return;
        }
    }
    _list->setSelectedIndex(-1);
}

void IdChooserDialog::updateOk ()
{
    _ok->setEnabled(_list->selectedIndex() != -1);
}

void IdChooserDialog::emitIdSelected ()
{
    emit idSelected(0, "");
}

void IdChooserDialog::populateList (const DescriptorList& values)
{
}

ZoneIdChooserDialog::ZoneIdChooserDialog (Session* parent, quint32 id, bool allowZero) :
    IdChooserDialog(parent, id, allowZero)
{
    QMetaObject::invokeMethod(parent->app()->databaseThread()->sceneRepository(), "findZones",
        Q_ARG(const QString&, ""), Q_ARG(quint32, 0), Q_ARG(const Callback&, Callback(_this,
            "populateList(DescriptorList)")));
}

SceneIdChooserDialog::SceneIdChooserDialog (Session* parent, quint32 id, bool allowZero) :
    IdChooserDialog(parent, id, allowZero)
{
    QMetaObject::invokeMethod(parent->app()->databaseThread()->sceneRepository(), "findScenes",
        Q_ARG(const QString&, ""), Q_ARG(quint32, 0), Q_ARG(const Callback&, Callback(_this,
            "populateList(DescriptorList)")));
}
