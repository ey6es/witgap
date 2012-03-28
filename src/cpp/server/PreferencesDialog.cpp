//
// $Id$

#include "PreferencesDialog.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"

// translate through the session
#define tr(...) session()->translate("CommandMenu", __VA_ARGS__)

/** The avatar expression. */
const QRegExp AvatarExp("[a-zA-Z0-9]?");

PreferencesDialog::PreferencesDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* icont = new Container(new TableLayout(2));
    icont->setBorder(new CharBorder(QMargins(1, 0, 1, 0), 0));
    addChild(icont);

    icont->addChild(new Label(tr("Avatar:")));
    icont->addChild(_avatar = new TextField(20, new RegExpDocument(
        AvatarExp, parent->record().avatar, 1)));
    connect(_avatar, SIGNAL(textChanged()), SLOT(updateApply()));

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_apply = new Button(tr("Apply")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok = new Button(tr("OK")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _apply, _ok));

    pack();
    center();
}

void PreferencesDialog::updateApply ()
{
    bool enable = !_avatar->text().isEmpty();
    _apply->setEnabled(enable);
    _ok->setEnabled(enable);
}

void PreferencesDialog::apply ()
{
    session()->setAvatar(_avatar->text().at(0));
}
