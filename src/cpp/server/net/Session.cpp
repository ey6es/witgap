//
// $Id$

#include <limits>

#include <QDate>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QRegExp>
#include <QtDebug>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Component.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/TextField.h"
#include "ui/Window.h"

using namespace std;

Session::Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token) :
    QObject(app->connectionManager()),
    _app(app),
    _connection(0),
    _id(id),
    _token(token),
    _lastWindowId(0),
    _moused(0),
    _focus(0)
{
    // send the session info back to the connection and activate it
    connection->setCookie("sessionId", QString::number(id, 16).rightJustified(16, '0'));
    connection->setCookie("sessionToken", token.toHex());
    setConnection(connection);

    showLogonDialog();
}

void Session::setConnection (Connection* connection)
{
    if (_connection != 0) {
        _connection->disconnect(this);
        _connection->deactivate(tr("Logged in elsewhere."));
    }
    _connection = connection;
    _displaySize = connection->displaySize();
    connect(_connection, SIGNAL(windowClosed()), SLOT(deleteLater()));
    connect(_connection, SIGNAL(destroyed()), SLOT(clearConnection()));
    connect(_connection, SIGNAL(mousePressed(int,int)), SLOT(dispatchMousePressed(int,int)));
    connect(_connection, SIGNAL(mouseReleased(int,int)), SLOT(dispatchMouseReleased(int,int)));
    connect(_connection, SIGNAL(keyPressed(int,QChar,bool)),
        SLOT(dispatchKeyPressed(int,QChar,bool)));
    connect(_connection, SIGNAL(keyReleased(int,QChar,bool)),
        SLOT(dispatchKeyReleased(int,QChar,bool)));
    _connection->activate();

    // clear the modifiers
    _modifiers = Qt::KeyboardModifiers();

    // readd the windows
    foreach (Window* window, findChildren<Window*>()) {
        window->resend();
    }
}

int Session::highestWindowLayer () const
{
    int highest = 0;
    foreach (Window* window, findChildren<Window*>()) {
        highest = qMax(highest, window->layer());
    }
    return highest;
}

void Session::setFocus (Component* component)
{
    if (_focus == component) {
        return;
    }
    if (_focus != 0) {
        _focus->disconnect(this, SLOT(clearFocus()));
        QFocusEvent event(QEvent::FocusOut);
        QCoreApplication::sendEvent(_focus, &event);
    }
    if ((_focus = component) != 0) {
        connect(_focus, SIGNAL(destroyed()), SLOT(clearFocus()));
        QFocusEvent event(QEvent::FocusIn);
        QCoreApplication::sendEvent(_focus, &event);
    }
}

/**
 * Helper for the various dialog functions.
 */
static Window* createDialog (Session* session, const QString& message, const QString& title)
{
    Window* window = new Window(session, session->highestWindowLayer());
    window->setModal(true);
    window->setBorder(title.isEmpty() ? new FrameBorder() : new TitledBorder(title));
    window->setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch));
    Label* label = new Label(message, Qt::AlignCenter);
    label->setPreferredSize(QSize(40, -1));
    window->addChild(label);
    return window;
}

void Session::showInfoDialog (const QString& message, const QString& title, const QString& dismiss)
{
    Window* window = createDialog(this, message, title);

    Button* button = new Button(dismiss.isEmpty() ? tr("OK") : dismiss);
    window->addChild(button);
    window->connect(button, SIGNAL(pressed()), SLOT(deleteLater()));
    button->requestFocus();
    window->pack();
    window->center();
}

void Session::showConfirmDialog (
    const QString& message, const Callback& callback, const QString& title,
    const QString& dismiss, const QString& accept)
{
    Window* window = createDialog(this, message, title);

    Button* cancel = new Button(dismiss.isEmpty() ? tr("Cancel") : dismiss);
    window->connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    Button* ok = new Button(accept.isEmpty() ? tr("OK") : accept);
    window->connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    (new CallbackObject(callback, window))->connect(ok, SIGNAL(pressed()), SLOT(invoke()));

    window->addChild(BoxLayout::createHBox(2, cancel, ok));

    ok->requestFocus();
    window->pack();
    window->center();
}

void Session::showInputDialog (
    const QString& message, const Callback& callback, const QString& title,
    const QString& dismiss, const QString& accept)
{
    Window* window = createDialog(this, message, title);

    TextField* field = new TextField();
    window->addChild(field);

    Button* cancel = new Button(dismiss.isEmpty() ? tr("Cancel") : dismiss);
    window->connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    Button* ok = new Button(accept.isEmpty() ? tr("OK") : accept);
    ok->connect(field, SIGNAL(enterPressed()), SLOT(doPress()));
    window->connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    (new CallbackObject(callback, window))->connect(ok, SIGNAL(pressed()), SLOT(invoke()));

    window->addChild(BoxLayout::createHBox(2, cancel, ok));

    field->requestFocus();
    window->pack();
    window->center();
}

void Session::showLogonDialog ()
{
    if (_connection == 0) {
        showLogonDialog("");
        return;
    }
    Connection::requestCookieMetaMethod().invoke(_connection,
        Q_ARG(const QString&, "username"), Q_ARG(const Callback&,
            Callback(this, "showLogonDialog(QString)")));
}

void Session::clearConnection ()
{
    _connection = 0;
}

void Session::clearMoused ()
{
    _moused = 0;
}

void Session::clearFocus ()
{
    _focus = 0;
}

void Session::dispatchMousePressed (int x, int y)
{
    // we shouldn't have a moused component, but let's make sure
    if (_moused != 0) {
        _moused->disconnect(this, SLOT(clearMoused()));
        _moused = 0;
    }

    // find the highest "hit"
    int hlayer = numeric_limits<int>::min();
    Component* target = 0;
    QPoint absolute(x, y), relative(x, y);
    foreach (Window* window, findChildren<Window*>()) {
        if (window->layer() < hlayer) {
            continue;
        }
        if (window->modal()) {
            // modal windows block anything underneath
            hlayer = window->layer();
            target = 0;
        }
        if (window->bounds().contains(absolute)) {
            QPoint rel;
            Component* comp = window->componentAt(absolute - window->bounds().topLeft(), &rel);
            if (comp != 0) {
                hlayer = window->layer();
                target = comp;
                relative = rel;
            }
        }
    }
    if (target != 0) {
        connect(_moused = target, SIGNAL(destroyed()), SLOT(clearMoused()));
    }
    QMouseEvent event(QEvent::MouseButtonPress, relative, absolute,
        Qt::LeftButton, Qt::LeftButton, _modifiers);
    QCoreApplication::sendEvent(target == 0 ? this : (QObject*)target, &event);
}

void Session::dispatchMouseReleased (int x, int y)
{
    QPoint absolute(x, y), relative(x, y);
    Component* target = _moused;
    if (_moused != 0) {
        relative -= _moused->absolutePos();
        _moused->disconnect(this, SLOT(clearMoused()));
        _moused = 0;
    }
    QMouseEvent event(QEvent::MouseButtonRelease, relative, absolute,
        Qt::LeftButton, Qt::NoButton, _modifiers);
    QCoreApplication::sendEvent(target == 0 ? this : (QObject*)target, &event);
}

/**
 * Helper function for modifier updates.
 */
Qt::KeyboardModifier getModifier (int key)
{
    switch (key) {
        case Qt::Key_Shift:
            return Qt::ShiftModifier;
        case Qt::Key_Control:
            return Qt::ControlModifier;
        case Qt::Key_Alt:
            return Qt::AltModifier;
        case Qt::Key_Meta:
            return Qt::MetaModifier;
        default:
            return Qt::NoModifier;
    }
}

void Session::dispatchKeyPressed (int key, QChar ch, bool numpad)
{
    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers |= modifier;
    }
    QKeyEvent event(QEvent::KeyPress, key,
        _modifiers | (numpad ? Qt::KeypadModifier : Qt::NoModifier),
        ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(_focus == 0 ? this : (QObject*)_focus, &event);
}

void Session::dispatchKeyReleased (int key, QChar ch, bool numpad)
{
    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers &= ~modifier;
    }
    QKeyEvent event(QEvent::KeyRelease, key,
        _modifiers | (numpad ? Qt::KeypadModifier : Qt::NoModifier),
        ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(_focus == 0 ? this : (QObject*)_focus, &event);
}

void Session::showLogonDialog (const QString& username)
{
    new LogonDialog(_app, this, username);
}

/** Expressions for partial and complete usernames. */
const QRegExp PARTIAL_USERNAME("[a-zA-Z0-9]{0,16}"), FULL_USERNAME("[a-zA-Z0-9]{3,16}");

/** Expressions for partial and complete passwords. */
const QRegExp PARTIAL_PASSWORD(".{0,16}"), FULL_PASSWORD(".{6,16}");

/** Month/day and year expressions. */
const QRegExp MONTH_DAY("\\d{0,2}"), YEAR("\\d{0,4}");

/** Partial and full email expressions (full from
 * http://www.regular-expressions.info/regexbuddy/email.html). */
const QRegExp PARTIAL_EMAIL("[a-zA-Z0-9._%-]*@?[a-zA-Z0-9.-]*\\.?[a-zA-Z]{0,4}");
const QRegExp FULL_EMAIL("[a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}");

LogonDialog::LogonDialog (ServerApp* app, Session* parent, const QString& username) :
    Window(parent, parent->highestWindowLayer()),
    _app(app),
    _logonBlocked(false)
{
    setModal(true);
    setBorder(new FrameBorder());
    setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignCenter, 1));
    setPreferredSize(QSize(43, -1));

    addChild(_label = new Label("", Qt::AlignHCenter));

    TableLayout* ilayout = new TableLayout(2);
    ilayout->stretchColumns() += 1;
    Container* icont = new Container(ilayout);
    addChild(icont);
    icont->addChild(new Label(tr("Username:")));
    _username = new TextField(20, new RegExpDocument(PARTIAL_USERNAME, username, 16));
    icont->addChild(_username);
    connect(_username, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Password:")));
    _password = new PasswordField(20, new RegExpDocument(PARTIAL_PASSWORD, "", 16));
    icont->addChild(_password);
    connect(_password, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Confirm Password:")));
    _confirmPassword = new PasswordField(20, new RegExpDocument(PARTIAL_PASSWORD, "", 16));
    icont->addChild(_confirmPassword);
    connect(_confirmPassword, SIGNAL(textChanged()), SLOT(updateLogon()));

    icont->addChild(new Label(tr("Date of Birth:")));
    _month = new TextField(3, new RegExpDocument(MONTH_DAY, "", 2), true);
    _month->setLabel(tr("MM"));
    connect(_month, SIGNAL(textChanged()), SLOT(updateLogon()));
    _day = new TextField(3, new RegExpDocument(MONTH_DAY, "", 2), true);
    _day->setLabel(tr("DD"));
    connect(_day, SIGNAL(textChanged()), SLOT(updateLogon()));
    _year = new TextField(5, new RegExpDocument(YEAR, "", 4), true);
    _year->setLabel(tr("YYYY"));
    connect(_year, SIGNAL(textChanged()), SLOT(updateLogon()));
    Container* dcont = BoxLayout::createHBox(
        1, _month, new Label("/"),_day, new Label("/"), _year);
    icont->addChild(dcont);

    icont->addChild(new Label(tr("Email (Optional):")));
    _email = new TextField(20, new RegExpDocument(PARTIAL_EMAIL));
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

    addChild(BoxLayout::createHBox(2, _cancel, _toggleCreateMode, _logon));

    _username->requestFocus();

    setCreateMode(username.isEmpty());
}

void LogonDialog::updateLogon ()
{
    bool enableLogon = !_logonBlocked && FULL_USERNAME.exactMatch(_username->text()) &&
        FULL_PASSWORD.exactMatch(_password->text());
    if (!_createMode) {
        _logon->setEnabled(enableLogon);
        return;
    }
    QString email = _email->text().trimmed();
    _logon->setEnabled(enableLogon && FULL_PASSWORD.exactMatch(_confirmPassword->text()) &&
        QDate(_year->text().toInt(), _month->text().toInt(), _day->text().toInt()).isValid() &&
        (email.isEmpty() || FULL_EMAIL.exactMatch(email)));
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
        QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "insertUser",
            Q_ARG(const QString&, _username->text()), Q_ARG(const QString&, _password->text()),
            Q_ARG(const QDate&, dob), Q_ARG(const QString&, _email->text().trimmed()),
            Q_ARG(const Callback&, Callback(this, "userMaybeInserted(quint32)")));

    } else {
        // block logon and send off the request
        _logonBlocked = true;
        _logon->setEnabled(false);
        QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "validateLogon",
            Q_ARG(const QString&, _username->text()), Q_ARG(const QString&, _password->text()),
            Q_ARG(const Callback&, Callback(this, "userMaybeInserted(quint32)")));
    }
}

void LogonDialog::userMaybeInserted (quint32 id)
{
    if (id == 0) {
        flashStatus(tr("Sorry, that account name is already taken.  Please choose another."));
        _logonBlocked = false;
        updateLogon();
        _username->requestFocus();
        return;
    }
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
