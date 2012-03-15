//
// $Id$

#include <QMetaObject>

#include "LogonDialog.h"
#include "ServerApp.h"
#include "admin/EditUserDialog.h"
#include "db/DatabaseThread.h"
#include "db/UserRepository.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Component.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "util/Callback.h"

// translate through the session
#define tr(...) session()->translate("EditUserDialog", __VA_ARGS__)

EditUserDialog::EditUserDialog (Session* parent) :
    Window(parent, parent->highestWindowLayer())
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));

    Container* scont = BoxLayout::createHStretchBox(1);
    addChild(scont);
    scont->addChild(new Label(tr("Username:")), BoxLayout::Fixed);
    scont->addChild(_username = new TextField(20, new RegExpDocument(PartialUsernameExp, "", 16)));
    connect(_username, SIGNAL(textChanged()), SLOT(updateSearch()));
    scont->addChild(_search = new Button(tr("Search")), BoxLayout::Fixed);
    connect(_search, SIGNAL(pressed()), SLOT(search()));
    _search->connect(_username, SIGNAL(enterPressed()), SLOT(doPress()));

    Container* icont = new Container(new TableLayout(2));
    addChild(icont);

    icont->addChild(new Label(tr("ID:")));
    icont->addChild(_id = new Label());

    icont->addChild(new Label(tr("Created:")));
    icont->addChild(_created = new Label());

    icont->addChild(new Label(tr("Last Online:")));
    icont->addChild(_lastOnline = new Label());

    icont->addChild(new Label(tr("New Username:")));
    icont->addChild(_newUsername = new TextField(20,
        new RegExpDocument(PartialUsernameExp, "", 16)));

    icont->addChild(new Label(tr("Password:")));
    icont->addChild(_password = new PasswordField(20,
        new RegExpDocument(PartialPasswordExp, "", 255)));

    icont->addChild(new Label(tr("Confirm Password:")));
    icont->addChild(_confirmPassword = new PasswordField(20,
        new RegExpDocument(PartialPasswordExp, "", 255)));

    icont->addChild(new Label(tr("Date of Birth:")));
    icont->addChild(_dob = new TextField());

    icont->addChild(new Label(tr("Email:")));
    icont->addChild(_email = new TextField(20, new RegExpDocument(PartialEmailExp, "", 255)));

    icont->addChild(new Label(tr("Banned:")));
    icont->addChild(_banned = new CheckBox());

    icont->addChild(new Label(tr("Admin:")));
    icont->addChild(_admin = new CheckBox());

    addChild(_status = new StatusLabel());

    Button* close = new Button(tr("Close"));
    connect(close, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_delete = new Button(tr("Delete")), SIGNAL(pressed()), SLOT(confirmDelete()));
    connect(_update = new Button(tr("Update")), SIGNAL(pressed()), SLOT(update()));
    addChild(BoxLayout::createHBox(2, close, _delete, _update));

    updateSearch();

    _username->requestFocus();
}

void EditUserDialog::updateSearch ()
{
    _id->container()->setVisible(false);
    _status->setVisible(false);
    _search->setEnabled(FullUsernameExp.exactMatch(_username->text()));
    _update->setEnabled(false);
    _delete->setEnabled(false);

    pack();
    center();
}

void EditUserDialog::search ()
{
    // send the request off to the database
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "loadUser",
        Q_ARG(const QString&, _username->text()),
        Q_ARG(const Callback&, Callback(this, "userMaybeLoaded(UserRecord)")));
}

void EditUserDialog::confirmDelete ()
{
    session()->showConfirmDialog(tr("Are you sure you want to delete this user?"),
        Callback(this, "reallyDelete()"));
}

void EditUserDialog::update ()
{
}

void EditUserDialog::userMaybeLoaded (const UserRecord& user)
{
    if (user.id == 0) {
        _status->setVisible(true);
        _status->setStatus(tr("No user exists with that name."), true);
        pack();
        center();
        return;
    }
    _id->container()->setVisible(true);
    _id->setText(QString::number(user.id));
    _created->setText(user.created.toString());
    _lastOnline->setText(user.lastOnline.toString());
    _newUsername->setText(user.name);
    _password->setText("");
    _confirmPassword->setText("");
    _dob->setText(user.dateOfBirth.toString("MM-dd-yyyy"));
    _email->setText(user.email);
    _banned->setSelected(user.flags.testFlag(UserRecord::Banned));
    _admin->setSelected(user.flags.testFlag(UserRecord::Admin));
    _update->setEnabled(true);
    _delete->setEnabled(true);

    pack();
    center();
}

void EditUserDialog::reallyDelete ()
{
}
