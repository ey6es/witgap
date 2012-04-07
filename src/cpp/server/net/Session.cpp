//
// $Id$

#include <limits>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QTranslator>
#include <QtDebug>

#include "LogonDialog.h"
#include "MainWindow.h"
#include "ServerApp.h"
#include "SettingsDialog.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/Connection.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/Window.h"

// translate through the session
#define tr(...) translate("Session", __VA_ARGS__)

using namespace std;

Session::Session (ServerApp* app, Connection* connection,
        const SessionRecord& record, const UserRecord& user) :
    CallableObject(app->connectionManager()),
    _app(app),
    _connection(0),
    _record(record),
    _lastWindowId(0),
    _cryptoCount(0),
    _mousePressed(false),
    _moused(0),
    _activeWindow(0),
    _translator(0),
    _user(user),
    _scene(0),
    _pawn(0)
{
    // send the session info back to the connection and activate it
    connection->setCookie("sessionId", QString::number(record.id, 16).rightJustified(16, '0'));
    connection->setCookie("sessionToken", record.token.toHex());

    // create the main window
    _mainWindow = new MainWindow(this);

    setConnection(connection);
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

    // activate encryption if necessary
    if (_cryptoCount > 0) {
        _connection->toggleCrypto();
    }

    // clear the modifiers
    _modifiers = Qt::KeyboardModifiers();

    // position the main window
    _mainWindow->setBounds(QRect(0, 0, _displaySize.width(), _displaySize.height()));

    // readd the windows
    foreach (Window* window, findChildren<Window*>()) {
        window->resend();
    }

    // check for a password reset request
    quint32 resetId = _connection->query().value("resetId", "0").toUInt();
    if (resetId != 0) {
        QByteArray token = QByteArray::fromHex(
            _connection->query().value("resetToken", "").toAscii());
        QMetaObject::invokeMethod(_app->databaseThread()->userRepository(),
            "validatePasswordReset", Q_ARG(quint32, resetId), Q_ARG(const QByteArray&, token),
            Q_ARG(const Callback&, Callback(_this, "passwordResetMaybeValidated(QVariant)")));
    }
}

QString Session::who () const
{
    return (_user.id == 0) ? QString::number(_record.id) : _user.name;
}

int Session::highestWindowLayer () const
{
    int highest = 0;
    foreach (Window* window, findChildren<Window*>()) {
        highest = qMax(highest, window->layer());
    }
    return highest;
}

void Session::setActiveWindow (Window* window)
{
    if (_activeWindow == window) {
        return;
    }
    if (_activeWindow != 0) {
        _activeWindow->disconnect(this, SLOT(clearActiveWindow()));
        _activeWindow->setActive(false);
    }
    if ((_activeWindow = window) != 0) {
        connect(_activeWindow, SIGNAL(destroyed()), SLOT(clearActiveWindow()));
        _activeWindow->setActive(true);
    }
}

void Session::updateActiveWindow ()
{
    if (_activeWindow != 0 && (_activeWindow->modal() || _activeWindow->focus() != 0) &&
            !belowModal(_activeWindow)) {
        return;
    }
    int hlayer = numeric_limits<int>::min();
    Window* hwindow = 0;
    foreach (Window* window, findChildren<Window*>()) {
        if (window->layer() < hlayer) {
            continue;
        }
        // we want the highest window that's modal or that has a focus
        if (window->modal() || window->focus() != 0) {
            hlayer = window->layer();
            hwindow = window;
        }
    }
    setActiveWindow(hwindow);
}

void Session::requestFocus (Component* component)
{
    Window* nwindow = component->window();
    nwindow->setFocus(component);

    // we can only make it the active window if there are no modal windows above
    if (!(nwindow == _activeWindow || belowModal(nwindow))) {
        setActiveWindow(nwindow);
    }
}

void Session::incrementCryptoCount ()
{
    if (_cryptoCount++ == 0 && _connection != 0) {
        Connection::toggleCryptoMetaMethod().invoke(_connection);
    }
}

void Session::decrementCryptoCount ()
{
    if (--_cryptoCount == 0 && _connection != 0) {
        Connection::toggleCryptoMetaMethod().invoke(_connection);
    }
}

/**
 * Helper for the various dialog functions.
 */
static Window* createDialog (Session* session, const QString& message, const QString& title)
{
    Window* window = new Window(session, session->highestWindowLayer(), true, true);
    window->setBorder(title.isEmpty() ? new FrameBorder() : new TitledBorder(title));
    window->setLayout(new BoxLayout(Qt::Vertical, BoxLayout::HStretch));
    Label* label = new Label(message, Qt::AlignCenter);
    label->setPreferredSize(QSize(45, -1));
    window->addChild(label);
    return window;
}

void Session::showInfoDialog (const QString& message, const QString& title, const QString& dismiss)
{
    Window* window = createDialog(this, message, title);

    Button* button = new Button(dismiss.isEmpty() ? tr("OK") : dismiss);
    window->addChild(button);
    window->connect(button, SIGNAL(pressed()), SLOT(deleteLater()));
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

    window->addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, ok));

    ok->requestFocus();
    window->pack();
    window->center();
}

/**
 * Helper class for input dialog; invokes the callback with the contents of the text field.
 */
class TextCallbackObject : public CallbackObject
{
public:

    /**
     * Creates a new text callback object.
     */
    TextCallbackObject (const Callback& callback, QObject* parent, TextField* field) :
        CallbackObject(callback, parent), _field(field) {}

protected:

    /**
     * Invokes the callback.
     */
    virtual void invoke () const { _callback.invoke(Q_ARG(const QString&, _field->text())); }

    /** The text field from which to obtain the text. */
    TextField* _field;
};

void Session::showInputDialog (
    const QString& message, const Callback& callback, const QString& title,
    const QString& dismiss, const QString& accept, Document* document, const QRegExp& acceptExp)
{
    Window* window = createDialog(this, message, title);

    TextField* field = new TextField(20, document);
    window->addChild(field);

    Button* cancel = new Button(dismiss.isEmpty() ? tr("Cancel") : dismiss);
    window->connect(cancel, SIGNAL(pressed()), SLOT(deleteLater()));

    Button* ok = new Button(accept.isEmpty() ? tr("OK") : accept);
    ok->connect(field, SIGNAL(enterPressed()), SLOT(doPress()));
    window->connect(ok, SIGNAL(pressed()), SLOT(deleteLater()));
    (new TextCallbackObject(callback, window, field))->connect(
        ok, SIGNAL(pressed()), SLOT(invoke()));
    new FieldExpEnabler(ok, field, acceptExp);

    window->addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, cancel, ok));

    window->pack();
    window->center();
}

void Session::showLogonDialog ()
{
    new LogonDialog(this, _connection == 0 ? "" : _connection->cookies().value("username", ""));
}

void Session::showLogoffDialog ()
{
    showConfirmDialog(tr("Are you sure you want to log off?"), Callback(_this, "logoff()"));
}

void Session::loggedOn (const UserRecord& user)
{
    qDebug() << "Logged on." << _record.id << user.name;

    _user = user;

    if (_connection != 0) {
        // set the username cookie on the client
        _connection->setCookieMetaMethod().invoke(_connection,
            Q_ARG(const QString&, "username"), Q_ARG(const QString&, user.name));
    }

    // set the user id and avatar in the session record
    _record.userId = user.id;
    _record.avatar = user.avatar;
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "updateSession",
        Q_ARG(const SessionRecord&, _record));
}

void Session::logoff ()
{
    qDebug() << "Logged off." << _record.id << _user.name;

    _user.id = 0;

    // clear the user id in the session record
    _record.userId = 0;
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "updateSession",
        Q_ARG(const SessionRecord&, _record));
}

void Session::createScene ()
{
    // insert the scene into the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "insertScene",
        Q_ARG(const QString&, tr("Untitled Scene")), Q_ARG(quint32, _user.id),
        Q_ARG(const Callback&, Callback(_this, "sceneCreated(quint32)")));
}

void Session::moveToScene (quint32 id)
{
    // resolve the scene via the manager
    QMetaObject::invokeMethod(_app->sceneManager(), "resolveScene",
        Q_ARG(quint32, id),
        Q_ARG(const Callback&, Callback(_this, "sceneMaybeResolved(QObject*)")));
}

void Session::setSettings (const QString& password, const QString& email, QChar avatar)
{
    // set and persist avatar in session record
    if (_record.avatar != avatar) {
        _record.avatar = avatar;
        QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "updateSession",
            Q_ARG(const SessionRecord&, _record));
    }

    // if logged on, set and persist in user record
    if (_user.id != 0) {
        _user.setPassword(password);
        _user.email = email;
        _user.avatar = avatar;
        QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "updateUser",
            Q_ARG(const UserRecord&, _user), Q_ARG(const Callback&, Callback()));
    }
}

QString Session::translate (
    const char* context, const char* sourceText, const char* disambiguation, int n)
{
    if (_translator != 0) {
        QString translated = _translator->translate(context, sourceText, disambiguation, n);
        if (!translated.isEmpty()) {
            return translated;
        }
    }
    return QString(sourceText);
}

bool Session::event (QEvent* e)
{
    if (e->type() != QEvent::KeyPress) {
        return QObject::event(e);
    }
    QKeyEvent* ke = (QKeyEvent*)e;
    if (ke->key() == Qt::Key_F2 && ke->modifiers() == Qt::NoModifier) {
        showInputDialog(tr("Please enter a brief bug description."),
            Callback(_this, "submitBugReport(QString)"));
        return true;

    } else {
        ke->ignore();
        return false;
    }
}

void Session::clearConnection ()
{
    _connection = 0;

    // clearing the connection implicitly releases keys and mouse button
    if (_mousePressed) {
        dispatchMouseReleased(0, 0);
    }
    foreach (int key, _keysPressed) {
        dispatchKeyReleased(key, 0, false);
    }
}

void Session::clearMoused ()
{
    _moused = 0;
}

void Session::clearActiveWindow ()
{
    _activeWindow = 0;

    // search for a new window to make active
    updateActiveWindow();
}

void Session::dispatchMousePressed (int x, int y)
{
    // note that we're pressed
    _mousePressed = true;

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
    // note that we're released
    _mousePressed = false;

    // dispatch to moused component, if any
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
    // note the press
    _keysPressed.insert(key);

    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers |= modifier;
    }
    QKeyEvent event(QEvent::KeyPress, key,
        _modifiers | (numpad ? Qt::KeypadModifier : Qt::NoModifier),
        ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(
        _activeWindow == 0 ? this : (QObject*)(_activeWindow->focus() == 0 ?
            _activeWindow : _activeWindow->focus()), &event);
}

void Session::dispatchKeyReleased (int key, QChar ch, bool numpad)
{
    // note the release
    _keysPressed.remove(key);

    // update our modifiers
    Qt::KeyboardModifier modifier = getModifier(key);
    if (modifier != Qt::NoModifier) {
        _modifiers &= ~modifier;
    }
    QKeyEvent event(QEvent::KeyRelease, key,
        _modifiers | (numpad ? Qt::KeypadModifier : Qt::NoModifier),
        ch == 0 ? QString() : QString(ch));
    QCoreApplication::sendEvent(
        _activeWindow == 0 ? this : (QObject*)(_activeWindow->focus() == 0 ?
            _activeWindow : _activeWindow->focus()), &event);
}

void Session::passwordResetMaybeValidated (const QVariant& result)
{
    UserRepository::LogonError error = (UserRepository::LogonError)result.toInt();
    if (error != UserRepository::NoError) {
        if (error == UserRepository::NoSuchUser) {
            qWarning() << "Received invalid password reset request.";
        }
        return;
    }
    UserRecord urec = qVariantValue<UserRecord>(result);
    qDebug() << "Activated password reset request." << urec.name << urec.email;
    if (_user.id != 0) {
        logoff();
    }
    loggedOn(urec);

    // open the settings dialog in order to change the password
    new SettingsDialog(this);
}

void Session::submitBugReport (const QString& description)
{
    qDebug() << "Submitting bug report." << who() << description;
    _app->sendMail(_app->bugReportAddress(), "Witgap bug report",
        who() + ": " + description, Callback());
}

void Session::sceneCreated (quint32 id)
{
    qDebug() << "Created scene." << _user.name << id;

    moveToScene(id);
}

void Session::sceneMaybeResolved (QObject* scene)
{
    if (scene == 0) {
        showInfoDialog(tr("Failed to resolve scene."));
        return;
    }
    // move the session to the scene thread
    if (_scene != 0) {
        emit willLeaveScene(_scene);

        _scene->removeSession(this);
        _scene = 0;
        _pawn = 0;
    }
    setParent(0);
    moveToThread(scene->thread());

    // continue the process in the scene thread
    QMetaObject::invokeMethod(this, "continueMovingToScene", Q_ARG(QObject*, scene));
}

void Session::continueMovingToScene (QObject* scene)
{
    _scene = static_cast<Scene*>(scene);
    _pawn = _scene->addSession(this);

    emit didEnterScene(_scene);
}

bool Session::belowModal (Window* window) const
{
    int layer = window->layer();
    bool after = false;
    foreach (Window* owindow, findChildren<Window*>()) {
        int olayer = owindow->layer();
        if (owindow == window) {
            after = true;
        } else if (owindow->modal() && (olayer > layer || olayer == layer && after)) {
            return true;
        }
    }
    return false;
}
