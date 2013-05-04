//
// $Id$

#include <QDate>
#include <QTranslator>
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
#include "ui/TextComponent.h"
#include "util/Callback.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("LogonDialog", __VA_ARGS__)

LogonDialog::LogonDialog (Session* parent, const QString& username, bool stayLoggedIn) :
    EncryptedWindow(parent, parent->highestWindowLayer(), true, true),
    _logonBlocked(false)
{
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
    setPreferredSize(QSize(43, -1));

    addChild(_label = new Label("", Qt::AlignHCenter));

    TableLayout* ilayout = new TableLayout(2);
    ilayout->stretchColumns() += 1;
    Container* icont = new Container(ilayout);
    addChild(icont);
    icont->addChild(new Label(tr("Username:")));
    _username = new TextField(20, new RegExpDocument(PartialUsernameExp,
        username.isEmpty() ? parent->user().name : username, MaxUsernameLength));
    icont->addChild(_username);
    connect(_username, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Password:")));
    _password = new PasswordField(20, new RegExpDocument(PartialPasswordExp));
    icont->addChild(_password);
    connect(_password, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Confirm Password:")));
    _confirmPassword = new PasswordField(20, new RegExpDocument(PartialPasswordExp));
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
    _email = new TextField(20, new RegExpDocument(PartialEmailExp));
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

    _forgotUsername = new Button(tr("Forgot Username"));
    connect(_forgotUsername, SIGNAL(pressed()), SLOT(forgotUsername()));
    _forgotPassword = new Button(tr("Forgot Password"));
    connect(_forgotPassword, SIGNAL(pressed()), SLOT(forgotPassword()));
    
    addChild(BoxLayout::createVBox(Qt::AlignCenter, 0,
        BoxLayout::createHBox(Qt::AlignCenter, 2, _cancel, _toggleCreateMode, _logon),
        BoxLayout::createHBox(Qt::AlignCenter, 2, _forgotUsername, _forgotPassword)));

    addChild(BoxLayout::createHBox(Qt::AlignCenter, 2,
        _stayLoggedIn = new CheckBox(tr("Stay Logged in after Leaving"), stayLoggedIn)));

    // force logon if appropriate for the current policy
    RuntimeConfig* config = parent->app()->runtimeConfig();
    setCreateMode(config->logonPolicy() == RuntimeConfig::Everyone && username.isEmpty());
    connect(config, SIGNAL(logonPolicyChanged(RuntimeConfig::LogonPolicy)),
        SLOT(updateForce(RuntimeConfig::LogonPolicy)));
    updateForce(config->logonPolicy());
}

void LogonDialog::updateForce (RuntimeConfig::LogonPolicy policy)
{
    bool everyone = (policy == RuntimeConfig::Everyone);
    setDeleteOnEscape(everyone);
    _cancel->setVisible(everyone);
    _toggleCreateMode->setVisible(everyone);
    if (_createMode && !everyone) {
        setCreateMode(false);
    }
    pack();
    center();
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
    Session* session = this->session();
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
        UserRecord nrec = session->user();
        nrec.name = _username->text();
        nrec.setPassword(_password->text());
        nrec.dateOfBirth = dob;
        nrec.email = _email->text().trimmed();
        QMetaObject::invokeMethod(
            session->app()->databaseThread()->userRepository(), "updateUser",
            Q_ARG(const UserRecord&, nrec), Q_ARG(const Callback&,
                Callback(_this, "userMaybeUpdated(UserRecord,bool)",
                    Q_ARG(const UserRecord&, nrec))));

    } else {
        // block logon and send off the request
        _logonBlocked = true;
        _logon->setEnabled(false);
        QMetaObject::invokeMethod(
            session->app()->databaseThread()->userRepository(), "validateLogon",
            Q_ARG(const UserRecord&, session->user()), Q_ARG(const QString&, _username->text()),
            Q_ARG(const QString&, _password->text()), Q_ARG(const Callback&,
                Callback(_this, "logonMaybeValidated(QVariant)")));
    }
}

void LogonDialog::forgotUsername ()
{
    session()->showInputDialog(tr("Enter your email address.  If we have it on record, we'll send "
        "a message with the associated username."), Callback(_this,
            "maybeSendUsernameEmail(QString)"), "", "", "",
        new RegExpDocument(PartialEmailExp), FullEmailExp);
}

void LogonDialog::forgotPassword ()
{
    session()->showInputDialog(tr("Enter your email address.  If we have it on record, we'll send "
        "a message with a link allowing you to reset your password."), Callback(_this,
            "maybeSendPasswordEmail(QString)"), "", "", "",
        new RegExpDocument(PartialEmailExp), FullEmailExp);
}

void LogonDialog::userMaybeUpdated (const UserRecord& user, bool success)
{
    if (success) {
        qDebug() << "Account created." << user.id << user.name << user.dateOfBirth << user.email;
        session()->loggedOn(user, _stayLoggedIn->selected());
        deleteLater();
        
    } else {
        flashStatus(tr("Sorry, that account name is already taken.  Please choose another."));
        _logonBlocked = false;
        updateLogon();
        _username->requestFocus();     
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
        case UserRepository::ServerClosed:
            flashStatus(tr("The server is currently closed.  Please try again later."));
            return;
        default:
            session()->loggedOn(qVariantValue<UserRecord>(result), _stayLoggedIn->selected());
            deleteLater();
            return;
    }
    _logonBlocked = false;
    updateLogon();
}

void LogonDialog::maybeSendUsernameEmail (const QString& email)
{
    // look up the username
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "loadUserByEmail",
        Q_ARG(const QString&, email), Q_ARG(const Callback&, Callback(_this,
            "maybeSendUsernameEmail(UserRecord)")));
}

void LogonDialog::maybeSendUsernameEmail (const UserRecord& urec)
{
    if (urec.id == 0) {
        flashStatus(tr("Sorry, that email was not in our database."));
        return;
    }
    qDebug() << "Sending username reminder email." << urec.name << urec.email;

    // send off the email
    session()->app()->sendMail(urec.email, tr("Witgap username reminder"),
        tr("Your username is %1.").arg(urec.name),
        Callback(_this, "emailMaybeSent(QString,QString)", Q_ARG(const QString&, urec.email)));
}

void LogonDialog::maybeSendPasswordEmail (const QString& email)
{
    // look up the user record
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "loadUserByEmail",
        Q_ARG(const QString&, email), Q_ARG(const Callback&, Callback(_this,
            "maybeSendPasswordEmail(UserRecord)")));
}

void LogonDialog::maybeSendPasswordEmail (const UserRecord& urec)
{
    if (urec.id == 0) {
        flashStatus(tr("Sorry, that email was not in our database."));
        return;
    }
    qDebug() << "Inserting password reset." << urec.name << urec.email;

    // insert the reset
    QMetaObject::invokeMethod(
        session()->app()->databaseThread()->userRepository(), "insertPasswordReset",
        Q_ARG(quint64, urec.id), Q_ARG(const Callback&, Callback(_this,
            "sendPasswordEmail(QString,QString)", Q_ARG(const QString&, urec.email))));
}

void LogonDialog::sendPasswordEmail (const QString& email, const QString& url)
{
    ServerApp* app = session()->app();
    app->sendMail(email, tr("Witgap password reset"),
        tr("To reset your password, visit %1").arg(url),
        Callback(_this, "emailMaybeSent(QString,QString)", Q_ARG(const QString&, email)));
}

void LogonDialog::emailMaybeSent (const QString& email, const QString& result)
{
    if (result.isEmpty()) {
        flashStatus(tr("Email sent."));
    } else {
        qWarning() << "Failed to send email." << email << result;
        flashStatus(tr("Sorry, we experienced an error sending the email."));
    }
}

void LogonDialog::setCreateMode (bool createMode)
{
    if (_createMode = createMode) {
        _label->setText(tr("Enter desired account details."));
        _toggleCreateMode->setLabel(tr("I Have an Account"));
        _logon->setLabel(tr("Create"));
        _password->disconnect(_logon);
    } else {
        _label->setText(tr("Enter account details."));
        _toggleCreateMode->setLabel(tr("Create New Account"));
        _logon->setLabel(tr("Logon"));
        _logon->connect(_password, SIGNAL(enterPressed()), SLOT(doPress()));
    }
    // everything after the fourth child is only for account creation
    const QList<Component*>& children = _username->container()->children();
    for (int ii = 4, nn = children.size(); ii < nn; ii++) {
        children.at(ii)->setVisible(createMode);
    }
    _status->setVisible(false);
    _forgotUsername->setVisible(!createMode);
    _forgotPassword->setVisible(!createMode);

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
