//
// $Id$

#include <QMetaObject>

#include "LogonDialog.h"
#include "ServerApp.h"
#include "admin/EditUserDialog.h"
#include "db/DatabaseThread.h"
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
    EncryptedWindow(parent, parent->highestWindowLayer(), true, true),
    _user(NoUser)
{
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
    icont->setBorder(new CharBorder(QMargins(1, 0, 1, 0), 0));
    addChild(icont);

    icont->addChild(new Label(tr("ID:")));
    icont->addChild(_id = new Label());

    icont->addChild(new Label(tr("Created:")));
    icont->addChild(_created = new Label());

    icont->addChild(new Label(tr("Last Online:")));
    icont->addChild(_lastOnline = new Label());

    icont->addChild(new Spacer(1, 1));
    icont->addChild(new Spacer(1, 1));

    icont->addChild(new Label(tr("New Username:")));
    icont->addChild(_newUsername = new TextField(20,
        new RegExpDocument(PartialUsernameExp, "", 16)));
    connect(_newUsername, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Password:")));
    icont->addChild(_password = new PasswordField(20,
        new RegExpDocument(PartialPasswordExp, "", 255)));
    connect(_password, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Confirm Password:")));
    icont->addChild(_confirmPassword = new PasswordField(20,
        new RegExpDocument(PartialPasswordExp, "", 255)));
    connect(_confirmPassword, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Date of Birth:")));
    icont->addChild(_dob = new TextField());
    connect(_dob, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Email:")));
    icont->addChild(_email = new TextField(20, new RegExpDocument(PartialEmailExp, "", 255)));
    connect(_email, SIGNAL(textChanged()), SLOT(updateApply()));

    icont->addChild(new Label(tr("Banned:")));
    icont->addChild(BoxLayout::createHBox(Qt::AlignLeft, 0, _banned = new CheckBox()));

    icont->addChild(new Label(tr("Admin:")));
    icont->addChild(BoxLayout::createHBox(Qt::AlignLeft, 0, _admin = new CheckBox()));

    addChild(_status = new StatusLabel());

    Button* cancel = new Button(tr("Cancel"));
    connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));
    connect(_delete = new Button(tr("Delete")), SIGNAL(pressed()), SLOT(confirmDelete()));
    connect(_apply = new Button(tr("Apply")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok = new Button(tr("OK")), SIGNAL(pressed()), SLOT(apply()));
    connect(_ok, SIGNAL(pressed()), SLOT(deleteLater()));
    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, _delete, _apply, _ok));

    updateSearch();
}

void EditUserDialog::updateSearch ()
{
    _id->container()->setVisible(false);
    _status->setVisible(false);
    _search->setEnabled(FullUsernameExp.exactMatch(_username->text()));
    _apply->setVisible(false);
    _ok->setVisible(false);
    _delete->setVisible(false);

    pack();
    center();

    _username->requestFocus();
}

void EditUserDialog::search ()
{
    // send the request off to the database
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "loadUser",
        Q_ARG(const QString&, _username->text()),
        Q_ARG(const Callback&, Callback(_this, "userMaybeLoaded(UserRecord)")));
}

void EditUserDialog::updateApply ()
{
    QString password = _password->text();
    QString email = _email->text();
    bool enable = FullUsernameExp.exactMatch(_newUsername->text()) &&
        password == _confirmPassword->text() &&
            (password.isEmpty() || FullPasswordExp.exactMatch(password)) &&
        QDate::fromString(_dob->text(), "MM-dd-yyyy").isValid() &&
        (email.isEmpty() || FullEmailExp.exactMatch(email));
    _apply->setEnabled(enable);
    _ok->setEnabled(enable);
}

void EditUserDialog::confirmDelete ()
{
    session()->showConfirmDialog(tr("Are you sure you want to delete this user?"),
        Callback(_this, "reallyDelete()"));
}

void EditUserDialog::apply ()
{
    // update the record fields
    _user.name = _newUsername->text();
    QString password = _password->text();
    if (!password.isEmpty()) {
        _user.setPassword(password);
    }
    _user.dateOfBirth = QDate::fromString(_dob->text(), "MM-dd-yyyy");
    _user.email = _email->text();
    _user.flags =
        (_banned->selected() ? UserRecord::Banned : UserRecord::NoFlag) |
        (_admin->selected() ? UserRecord::Admin : UserRecord::NoFlag);

    // send the request off to the database
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "updateUser",
        Q_ARG(const UserRecord&, _user),
        Q_ARG(const Callback&, Callback(_this, "userMaybeUpdated(bool)")));
}

void EditUserDialog::userMaybeLoaded (const UserRecord& user)
{
    if ((_user = user).id == 0) {
        _status->setVisible(true);
        _status->setStatus(tr("No user exists with that name."), true);
        pack();
        center();
        _username->requestFocus();
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
    _apply->setVisible(true);
    _apply->setEnabled(true);
    _ok->setVisible(true);
    _ok->setEnabled(true);
    _delete->setVisible(true);

    pack();
    center();

    _newUsername->requestFocus();
}

void EditUserDialog::reallyDelete ()
{
    // send the request off to the database
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "deleteUser",
        Q_ARG(quint32, _user.id));

    // reset the interface
    updateSearch();
}

void EditUserDialog::userMaybeUpdated (bool updated)
{
    _status->setVisible(true);
    if (updated) {
        _status->setStatus(tr("User updated."), true);
    } else {
        _status->setStatus(tr("The requested name was not available."), true);
        _newUsername->requestFocus();
    }
    pack();
    center();
}
