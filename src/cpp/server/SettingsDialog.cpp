//
// $Id$

#include "LogonDialog.h"
#include "SettingsDialog.h"
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

SettingsDialog::SettingsDialog (Session* parent) :
    EncryptedWindow(parent, parent->highestWindowLayer(), true, true)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* icont = new Container(new TableLayout(2));
    icont->setBorder(new CharBorder(QMargins(1, 0, 1, 0), 0));
    addChild(icont);

    icont->addChild(new Label(tr("Password:")));
    icont->addChild(_password = new PasswordField(
        20, new RegExpDocument(PartialPasswordExp)));
    _password->setLabel(tr("(Use Current)"));
    connect(_password, SIGNAL(textChanged()), SLOT(updateApply()));

    const UserRecord& user = parent->user();

    icont->addChild(new Label(tr("Confirm Password:")));
    icont->addChild(_confirmPassword = new PasswordField(
        20, new RegExpDocument(PartialPasswordExp)));
    connect(_confirmPassword, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Email (Optional):")));
    icont->addChild(_email = new TextField(20, new RegExpDocument(PartialEmailExp, user.email)));
    connect(_email, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Avatar:")));
    icont->addChild(_avatar = new TextField(20, new RegExpDocument(
        AvatarExp, parent->record().avatar, 1)));
    connect(_avatar, SIGNAL(textChanged()), SLOT(updateApply()));

    // if there's no user, hide the first three lines
    if (user.id == 0) {
        const QList<Component*>& children = icont->children();
        for (int ii = 0, nn = 6; ii < nn; ii++) {
            children.at(ii)->setVisible(false);
        }
    }

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_apply = new Button(tr("Apply")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok = new Button(tr("OK")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _apply, _ok));

    pack();
    center();
}

void SettingsDialog::updateApply ()
{
    QString email = _email->text().trimmed();
    bool enable = (_password->text().isEmpty() || FullPasswordExp.exactMatch(_password->text())) &&
        _confirmPassword->text() == _password->text() &&
        (email.isEmpty() || FullEmailExp.exactMatch(email)) &&
        !_avatar->text().isEmpty();
    _apply->setEnabled(enable);
    _ok->setEnabled(enable);
}

void SettingsDialog::apply ()
{
    session()->setSettings(_password->text(), _email->text(), _avatar->text().at(0));
}
