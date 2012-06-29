//
// $Id$

#include <QTranslator>

#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/ChooserDialog.h"
#include "ui/Layout.h"
#include "ui/ScrollingList.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("ChooserDialog", __VA_ARGS__)

ChooserDialog::ChooserDialog (Session* parent, bool allowEmpty) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* ncont = BoxLayout::createHStretchBox(1);
    addChild(ncont);
    ncont->addChild(new Label(tr("Name:")), BoxLayout::Fixed);
    ncont->addChild(_name = new TextField(20, new Document("", 255)));
    connect(_name, SIGNAL(textChanged()), SLOT(updateSelection()));

    addChild(_list = new ScrollingList());
    if (!allowEmpty) {
        connect(_list, SIGNAL(selectionChanged()), SLOT(updateOk()));
    }

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    _ok = new Button(tr("OK"));
    _ok->connect(_name, SIGNAL(enterPressed()), SLOT(doPress()));
    _ok->connect(_list, SIGNAL(enterPressed()), SLOT(doPress()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _ok));

    pack();
    center();
}

void ChooserDialog::updateSelection ()
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

void ChooserDialog::updateOk ()
{
    _ok->setEnabled(_list->selectedIndex() != -1);
}

ZoneChooserDialog::ZoneChooserDialog (Session* parent, bool allowEmpty) :
    ChooserDialog(parent, allowEmpty)
{
}

SceneChooserDialog::SceneChooserDialog (Session* parent, bool allowEmpty) :
    ChooserDialog(parent, allowEmpty)
{
}
