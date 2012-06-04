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
#include "RuntimeConfig.h"
#include "ServerApp.h"
#include "SettingsDialog.h"
#include "actor/Pawn.h"
#include "db/DatabaseThread.h"
#include "db/SceneRepository.h"
#include "net/ConnectionManager.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/Zone.h"
#include "ui/Border.h"
#include "ui/Button.h"
#include "ui/Label.h"
#include "ui/Layout.h"
#include "ui/Window.h"

// translate through the translator
#define tr(...) _translator->translate("Session", __VA_ARGS__)

using namespace std;

// register our types with the metatype system
int sessionPointerType = qRegisterMetaType<Session*>("Session*");

Session::Session (ServerApp* app, const SharedConnectionPointer& connection,
        const SessionRecord& record, const UserRecord& user) :
    CallableObject(app->connectionManager()),
    _app(app),
    _record(record),
    _lastWindowId(0),
    _cryptoCount(0),
    _mousePressed(false),
    _moused(0),
    _activeWindow(0),
    _translator(app->translators().value(locale())),
    _user(user),
    _scene(0),
    _pawn(0)
{
    // add info on all peers
    SessionInfo info = { _record.id, _record.name, _app->peerManager()->record().name };
    _info = info;
    _app->peerManager()->invoke(_app->peerManager(), "sessionAdded(SessionInfo)",
        Q_ARG(const SessionInfo&, info));

    // send the session info back to the connection
    connection->setCookie("sessionId", QString::number(record.id, 16).rightJustified(16, '0'));
    connection->setCookie("sessionToken", record.token.toHex());

    // create the main window
    _mainWindow = new MainWindow(this);

    // and the chat windows
    _chatWindow = new ChatWindow(this);
    _chatEntryWindow = new ChatEntryWindow(this);

    setConnection(connection);

    // force logon if the server isn't open
    if (user.id == 0 && !_app->runtimeConfig()->logonPolicy() != RuntimeConfig::Everyone) {
        showLogonDialog(true);
    }
}

void Session::maybeSetConnection (
    const SharedConnectionPointer& connection, const QByteArray& token, const Callback& callback)
{
    if (_connection || _record.token != token) {
        callback.invoke(Q_ARG(bool, false));
        return;
    }
    setConnection(connection);
    callback.invoke(Q_ARG(bool, true));
}

QString Session::who () const
{
    return _record.name;
}

QString Session::locale () const
{
    return "en";
}

int Session::highestWindowLayer () const
{
    int highest = 0;
    foreach (Window* window, _windows) {
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
        _activeWindow->setActive(false);
    }
    if ((_activeWindow = window) != 0) {
        _activeWindow->setActive(true);
    }
}

void Session::updateActiveWindow ()
{
    if (_activeWindow != 0 && (_activeWindow->modal() || _activeWindow->focus() != 0) &&
            _activeWindow->visible() && !belowModal(_activeWindow)) {
        return;
    }
    int hlayer = numeric_limits<int>::min();
    Window* hwindow = 0;
    foreach (Window* window, _windows) {
        if (!window->visible() || window->layer() < hlayer) {
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
    if (nwindow->visible() && !(nwindow == _activeWindow || belowModal(nwindow))) {
        setActiveWindow(nwindow);
    }
}

void Session::incrementCryptoCount ()
{
    if (_cryptoCount++ == 0 && _connection) {
        Connection::toggleCryptoMetaMethod().invoke(_connection.data());
    }
}

void Session::decrementCryptoCount ()
{
    if (--_cryptoCount == 0 && _connection) {
        Connection::toggleCryptoMetaMethod().invoke(_connection.data());
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

void Session::loggedOn (const UserRecord& user)
{
    qDebug() << "Logged on." << _record.id << user.name;

    _user = user;

    if (_connection) {
        // set the username cookie on the client
        Connection::setCookieMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, "username"), Q_ARG(const QString&, user.name));
    }

    // set the user id and name/avatar in the session record
    _record.userId = user.id;
    QString oname = _record.name;
    _record.name = user.name;
    _record.avatar = user.avatar;
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "updateSession",
        Q_ARG(const SessionRecord&, _record));

    // update mappings
    nameChanged(oname);
}

void Session::logoff ()
{
    qDebug() << "Logged off." << _record.id << _user.name;

    _user = NoUser;

    // clear the user id and name in the session record
    _record.userId = 0;
    QMetaObject::invokeMethod(_app->databaseThread()->sessionRepository(), "logoffSession",
        Q_ARG(quint64, _record.id), Q_ARG(const Callback&, Callback(_this, "loggedOff(QString)")));
}

void Session::moveToScene (quint32 id)
{
    // resolve the scene via the manager
    QMetaObject::invokeMethod(_app->sceneManager(), "resolveScene",
        Q_ARG(quint32, id),
        Q_ARG(const Callback&, Callback(_this, "sceneMaybeResolved(QObject*)")));
}

void Session::moveToZone (quint32 id)
{
    // resolve the zone via the manager
    QMetaObject::invokeMethod(_app->sceneManager(), "resolveZone",
        Q_ARG(quint32, id),
        Q_ARG(const Callback&, Callback(_this, "zoneMaybeResolved(QObject*)")));
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

void Session::say (const QString& message, ChatWindow::SpeakMode mode)
{
    if (mode == ChatWindow::BroadcastMode) {
        _app->peerManager()->invoke(_app->connectionManager(), "broadcast(QString,QString)",
            Q_ARG(const QString&, _record.name), Q_ARG(const QString&, message));
        return;
    }
    if (_scene != 0 && _pawn != 0) {
        // replace double with single quotes to prevent spoofing
        QString msg = message;
        msg.replace('\"', '\'');
        _scene->say(_pawn->position(), _record.name, msg, mode);
    }
}

void Session::tell (const QString& recipient, const QString& message)
{
    _app->peerManager()->invokeSession(recipient, _app->connectionManager(), "tell",
        Q_ARG(const QString&, _record.name), Q_ARG(const QString&, message),
        Q_ARG(const QString&, recipient), Q_ARG(const Callback&, Callback(_this,
            "maybeTold(QString,QString,bool)", Q_ARG(const QString&, recipient),
            Q_ARG(const QString&, message))));
}

void Session::submitBugReport (const QString& description)
{
    qDebug() << "Submitting bug report." << who() << description;
    _app->sendMail(_app->bugReportAddress(), "Witgap bug report",
        who() + ": " + description, Callback());
}

void Session::windowCreated (Window* window)
{
    _windows.append(window);

    // if modal, we need to update the active window first thing
    if (window->modal()) {
        updateActiveWindow();
    }
}

void Session::windowDestroyed (Window* window)
{
    _windows.removeOne(window);

    // clear the active window if necessary
    if (_activeWindow == window) {
        _activeWindow = 0;
        updateActiveWindow();
    }
}

bool Session::event (QEvent* e)
{
    if (e->type() != QEvent::KeyPress) {
        return QObject::event(e);
    }
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);
    if (ke->key() == Qt::Key_F2 && ke->modifiers() == Qt::NoModifier) {
        showInputDialog(tr("Please enter a brief bug description."),
            Callback(_this, "submitBugReport(QString)"));
        return true;

    } else {
        ke->ignore();
        return false;
    }
}

void Session::showLogonDialog (bool force, bool allowCreate)
{
    new LogonDialog(this, _connection ? _connection->cookies().value("username", "") : "",
        force, allowCreate);
}

void Session::showLogoffDialog ()
{
    showConfirmDialog(tr("Are you sure you want to log off?"), Callback(_this, "logoff()"));
}

void Session::createScene ()
{
    // insert the scene into the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "insertScene",
        Q_ARG(const QString&, tr("Untitled Scene")), Q_ARG(quint32, _user.id),
        Q_ARG(const Callback&, Callback(_this, "sceneCreated(quint32)")));
}

void Session::createZone ()
{
    // insert the zone into the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "insertZone",
        Q_ARG(const QString&, tr("Untitled Zone")), Q_ARG(quint32, _user.id),
        Q_ARG(const Callback&, Callback(_this, "zoneCreated(quint32)")));
}

void Session::clearConnection ()
{
    _connection.clear();

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
    foreach (Window* window, _windows) {
        if (!window->visible() || window->layer() < hlayer) {
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

void Session::close ()
{
    leaveScene();

    // let the connection manager update its mappings
    QMetaObject::invokeMethod(_app->connectionManager(), "sessionClosed",
        Q_ARG(quint64, _record.id), Q_ARG(const QString&, _record.name));

    // remove info on all peers
    _app->peerManager()->invoke(_app->peerManager(), "sessionRemoved(quint64)",
        Q_ARG(quint64, _record.id));

    qDebug() << "Session closed." << _record.id << _record.name;
}

void Session::setConnection (const SharedConnectionPointer& connection)
{
    // make sure it hasn't already been closed
    Connection* conn = connection.data();
    connect(conn, SIGNAL(closed()), SLOT(clearConnection()));
    if (!conn->isOpen()) {
        return;
    }

    _connection = connection;

    _displaySize = conn->displaySize();
    connect(conn, SIGNAL(windowClosed()), SLOT(close()));
    connect(conn, SIGNAL(mousePressed(int,int)), SLOT(dispatchMousePressed(int,int)));
    connect(conn, SIGNAL(mouseReleased(int,int)), SLOT(dispatchMouseReleased(int,int)));
    connect(conn, SIGNAL(keyPressed(int,QChar,bool)),
        SLOT(dispatchKeyPressed(int,QChar,bool)));
    connect(conn, SIGNAL(keyReleased(int,QChar,bool)),
        SLOT(dispatchKeyReleased(int,QChar,bool)));
    QMetaObject::invokeMethod(conn, "activate");

    // activate encryption if necessary
    if (_cryptoCount > 0) {
        Connection::toggleCryptoMetaMethod().invoke(conn);
    }

    // clear the modifiers
    _modifiers = Qt::KeyboardModifiers();

    // position the main and chat windows
    int width = _displaySize.width(), height = _displaySize.height();
    _mainWindow->setBounds(QRect(0, 0, width, height));
    _chatWindow->setBounds(QRect(3, height - 13, 40, 10));
    _chatEntryWindow->setBounds(QRect(3, height - 2, 40, 1));

    // readd the windows
    foreach (Window* window, _windows) {
        window->maybeResend();
    }

    // check for a password reset request
    quint32 resetId = conn->query().value("resetId", "0").toUInt();
    if (resetId != 0) {
        QByteArray token = QByteArray::fromHex(conn->query().value("resetToken", "").toAscii());
        QMetaObject::invokeMethod(_app->databaseThread()->userRepository(),
            "validatePasswordReset", Q_ARG(quint32, resetId), Q_ARG(const QByteArray&, token),
            Q_ARG(const Callback&, Callback(_this, "passwordResetMaybeValidated(QVariant)")));
    }
}

void Session::passwordResetMaybeValidated (const QVariant& result)
{
    UserRepository::LogonError error = (UserRepository::LogonError)result.toInt();
    if (error != UserRepository::NoError) {
        return; // no need to issue a warning; they might just have reloaded the page
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

void Session::sceneCreated (quint32 id)
{
    qDebug() << "Created scene." << _record.name << id;

    moveToScene(id);
}

void Session::zoneCreated (quint32 id)
{
    qDebug() << "Created zone." << _record.name << id;

    moveToZone(id);
}

void Session::sceneMaybeResolved (QObject* scene)
{
    if (scene == 0) {
        showInfoDialog(tr("Failed to resolve scene."));
        return;
    }
    // move the session to the scene thread
    leaveScene();
    setParent(0);
    moveToThread(scene->thread());

    // continue the process in the scene thread
    QMetaObject::invokeMethod(this, "continueMovingToScene", Q_ARG(QObject*, scene));
}

void Session::leaveScene ()
{
    if (_scene != 0) {
        emit willLeaveScene(_scene);

        _scene->removeSession(this);
        _scene = 0;
        _pawn = 0;
    }
}

void Session::continueMovingToScene (QObject* scene)
{
    _scene = static_cast<Scene*>(scene);
    _pawn = _scene->addSession(this);

    emit didEnterScene(_scene);
}

void Session::zoneMaybeResolved (QObject* zone)
{
    if (zone == 0) {
        showInfoDialog(tr("Failed to resolve zone."));
        return;
    }

    // reserve a place in an instance
    QMetaObject::invokeMethod(zone, "reserveInstancePlace",
        Q_ARG(const Callback&, Callback(_this, "instancePlaceReserved(QObject*)")));
}

void Session::instancePlaceReserved (QObject* instance)
{
    // move the session to the instance thread
    leaveZone();
    setParent(0);
    moveToThread(instance->thread());

    // continue the process in the instance thread
    QMetaObject::invokeMethod(this, "continueMovingToZone", Q_ARG(QObject*, instance));
}

void Session::leaveZone ()
{
    if (_instance != 0) {
        _instance = 0;
    }
}

void Session::continueMovingToZone (QObject* instance)
{
    _instance = static_cast<Instance*>(instance);
}

void Session::maybeTold (const QString& recipient, const QString& message, bool success)
{
    _chatWindow->display(success ? tr("You tell %1, \"%2\"").arg(recipient, message) :
        tr("There is no one online named %1.").arg(recipient));
}

void Session::loggedOff (const QString& name)
{
    // set the name in the session record
    QString oname = _record.name;
    _record.name = name;

    // update mappings
    nameChanged(oname);
}

void Session::nameChanged (const QString& oldName)
{
    // let the connection manager update its mappings
    QMetaObject::invokeMethod(_app->connectionManager(), "sessionNameChanged",
        Q_ARG(const QString&, oldName), Q_ARG(const QString&, _record.name));

    // update the info on all peers
    _info.name = _record.name;
    _app->peerManager()->invoke(_app->peerManager(), "sessionUpdated(SessionInfo)",
        Q_ARG(const SessionInfo&, _info));
}

bool Session::belowModal (Window* window) const
{
    int layer = window->layer();
    bool after = false;
    foreach (Window* owindow, _windows) {
        if (!owindow->visible()) {
            continue;
        }
        int olayer = owindow->layer();
        if (owindow == window) {
            after = true;
        } else if (owindow->modal() && (olayer > layer || olayer == layer && after)) {
            return true;
        }
    }
    return false;
}
