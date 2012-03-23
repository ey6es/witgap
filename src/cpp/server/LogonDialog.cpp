//
// $Id$

#include <QDate>
#include <QtDebug>

#include "LogonDialog.h"
#include "ServerApp.h"
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
#define tr(...) session()->translate("LogonDialog", __VA_ARGS__)

LogonDialog::LogonDialog (Session* parent, const QString& username) :
    Window(parent, parent->highestWindowLayer()),
    _logonBlocked(false)
{
    setModal(true);
    setDeleteOnEscape(true);
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
    setPreferredSize(QSize(43, -1));

    addChild(_label = new Label("", Qt::AlignHCenter));

    TableLayout* ilayout = new TableLayout(2);
    ilayout->stretchColumns() += 1;
    Container* icont = new Container(ilayout);
    addChild(icont);
    icont->addChild(new Label(tr("Username:")));
    _username = new TextField(20, new RegExpDocument(PartialUsernameExp, username, 16));
    icont->addChild(_username);
    connect(_username, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Password:")));
    _password = new PasswordField(20, new RegExpDocument(PartialPasswordExp, "", 255));
    icont->addChild(_password);
    connect(_password, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Confirm Password:")));
    _confirmPassword = new PasswordField(20, new RegExpDocument(PartialPasswordExp, "", 255));
    icont->addChild(_confirmPassword);
    connect(_confirmPassword, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Date of Birth:")));
    _month = new TextField(3, new RegExpDocument(MonthDayExp, "", 2), true);
    _month->setLabel(tr("MM"));
    connect(_month, SIGNAL(textChanged()), SLOT(updateLogon()));
    _month->connect(_month, SIGNAL(textFull()), SLOT(transferFocus()));
    _day = new TextField(3, new RegExpDocument(MonthDayExp, "", 2), true);
    _day->setLabel(tr("DD"));
    connect(_day, SIGNAL(textChanged()), SLOT(updateLogon()));
    _day->connect(_day, SIGNAL(textFull()), SLOT(transferFocus()));
    _year = new TextField(5, new RegExpDocument(YearExp, "", 4), true);
    _year->setLabel(tr("YYYY"));
    connect(_year, SIGNAL(textChanged()), SLOT(updateLogon()));
    _year->connect(_year, SIGNAL(textFull()), SLOT(transferFocus()));
    Container* dcont = BoxLayout::createHBox(
        Qt::AlignCenter, 1, _month, new Label("/"),_day, new Label("/"), _year);
    icont->addChild(dcont);

    icont->addChild(new Label(tr("Email (Optional):")));
    _email = new TextField(20, new RegExpDocument(PartialEmailExp, "", 255));
    icont->addChild(_email);
    connect(_email, SIGNAL(textChanged()), SLOT(updateLogon()));

    addChild(_status = new StatusLabel());

    _cancel = new Button(tr("Cancel"));
    connect(_cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    _toggleCreateMode = new Button();
    connect(_toggleCreateMode, SIGNAL(pressed()), SLOT(toggleCreateMode()));

    _logon = new Button(tr("Logon"));
    connect(_logon, SIGNAL(pressed()), SLOT(logon()));
    _logon->connect(_email, SIGNAL(enterPressed()), SLOT(doPress()));

    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, _cancel, _toggleCreateMode, _logon));

    _username->requestFocus();

    setCreateMode(username.isEmpty());
}

void LogonDialog::updateLogon ()
{
    bool enableLogon = !_logonBlocked && FullUsernameExp.exactMatch(_username->text()) &&
        FullPasswordExp.exactMatch(_password->text());
    if (!_createMode) {
        _logon->setEnabled(enableLogon);
        return;
    }
    QString email = _email->text().trimmed();
    _logon->setEnabled(enableLogon && FullPasswordExp.exactMatch(_confirmPassword->text()) &&
        QDate(_year->text().toInt(), _month->text().toInt(), _day->text().toInt()).isValid() &&
        (email.isEmpty() || FullEmailExp.exactMatch(email)));
}

void LogonDialog::logon ()
{
    // pass the request on to the database
    if (_createMode) {
        // check the passwords
        if (_password->text() != _confirmPassword->text()) {
            flashStatus(tr("The passwords you entered do not match."));
            _password->requestFocus();
            return;
        }

        // check the date of birth
        _logonBlocked = true;
        _logon->setEnabled(false);
        QDate dob(_year->text().toInt(), _month->text().toInt(), _day->text().toInt());
        if (dob > QDate::currentDate().addYears(-13)) {
            flashStatus(tr("Sorry, you must be at least 13 to create an account."));
            _cancel->requestFocus();
            return;
        }

        // send the request off to the database
        QMetaObject::invokeMethod(
            session()->app()->databaseThread()->userRepository(), "insertUser",
            Q_ARG(const QString&, _username->text()), Q_ARG(const QString&, _password->text()),
            Q_ARG(const QDate&, dob), Q_ARG(const QString&, _email->text().trimmed()),
            Q_ARG(const Callback&, Callback(_this, "userMaybeInserted(UserRecord)")));

    } else {
        // block logon and send off the request
        _logonBlocked = true;
        _logon->setEnabled(false);
        QMetaObject::invokeMethod(
            session()->app()->databaseThread()->userRepository(), "validateLogon",
            Q_ARG(const QString&, _username->text()), Q_ARG(const QString&, _password->text()),
            Q_ARG(const Callback&, Callback(_this, "logonMaybeValidated(QVariant)")));
    }
}

void LogonDialog::userMaybeInserted (const UserRecord& user)
{
    if (user.id == 0) {
        flashStatus(tr("Sorry, that account name is already taken.  Please choose another."));
        _logonBlocked = false;
        updateLogon();
        _username->requestFocus();
    } else {
        qDebug() << "Account created." << user.id << user.name << user.dateOfBirth << user.email;

        session()->loggedOn(user);
        deleteLater();
    }
}

void LogonDialog::logonMaybeValidated (const QVariant& result)
{
    switch (result.toInt()) {
        case UserRepository::NoSuchUser:
            flashStatus(tr("No account exists with that username."));
            _username->requestFocus();
            break;
        case UserRepository::WrongPassword:
            flashStatus(tr("The password you have entered is incorrect."));
            _password->requestFocus();
            break;
        case UserRepository::Banned:
            flashStatus(tr("Your account has been banned from the game."));
            return;
        default:
            session()->loggedOn(qVariantValue<UserRecord>(result));
            deleteLater();
            return;
    }
    _logonBlocked = false;
    updateLogon();
}

void LogonDialog::setCreateMode (bool createMode)
{
    if (_createMode = createMode) {
        _label->setText(tr("Enter desired account details."));
        _toggleCreateMode->setText(tr("I Have an Account"));
        _logon->setText(tr("Create"));
        _password->disconnect(_logon);
    } else {
        _label->setText(tr("Enter account details."));
        _toggleCreateMode->setText(tr("Create New Account"));
        _logon->setText(tr("Logon"));
        _logon->connect(_password, SIGNAL(enterPressed()), SLOT(doPress()));
    }
    // everything after the fourth child is only for account creation
    const QList<Component*>& children = _username->container()->children();
    for (int ii = 4, nn = children.size(); ii < nn; ii++) {
        children.at(ii)->setVisible(createMode);
    }
    _status->setVisible(false);
    _username->requestFocus();
    updateLogon();
    pack();
    center();
}

void LogonDialog::flashStatus (const QString& status)
{
    _status->setVisible(true);
    _status->setStatus(status, true);
    pack();
    center();
}
