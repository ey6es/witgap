//
// $Id$

#include <limits>

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QTimer>
#include <QTranslator>
#include <QUrl>
#include <QtDebug>

#include "LogonDialog.h"
#include "MainWindow.h"
#include "RuntimeConfig.h"
#include "ServerApp.h"
#include "SettingsDialog.h"
#include "actor/Pawn.h"
#include "db/ActorRepository.h"
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
#include "ui/ResourceChooserDialog.h"
#include "ui/Window.h"

// translate through the translator
#define tr(...) _translator->translate("Session", __VA_ARGS__)

using namespace std;

// register our types with the metatype system
int sessionPointerType = qRegisterMetaType<Session*>("Session*");

/** The time to allow a disconnected session to linger before closing it. */
static const int DisconnectTimeout = 5 * 60 * 1000;

Session::Session (ServerApp* app, const SharedConnectionPointer& connection,
        const UserRecord& user, const SessionTransfer& transfer) :
    CallableObject(app->connectionManager()),
    _app(app),
    _closeTimer(new QTimer(this)),
    _lastWindowId(0),
    _cryptoCount(0),
    _evaluator(0),
    _mousePressed(false),
    _moused(0),
    _activeWindow(0),
    _translator(app->translators().value(locale())),
    _user(user),
    _instance(0),
    _scene(0),
    _pawn(0)
{
    // add info on all peers
    const QString& peer = _app->peerManager()->record().name;
    SessionInfo info = { _user.id, _user.name, peer };
    _info = info;
    _app->peerManager()->invoke(_app->peerManager(), "sessionAdded(SessionInfo)",
        Q_ARG(const SessionInfo&, info));

    // create the main window
    _mainWindow = new MainWindow(this);

    // and the chat windows
    _chatWindow = new ChatWindow(this);
    _chatEntryWindow = new ChatEntryWindow(this, transfer.chatCommandHistory);

    // start the disconnect timer
    connect(_closeTimer, SIGNAL(timeout()), SLOT(shutdown()));
    _closeTimer->start(DisconnectTimeout);

    // install the connection, if available
    if (connection) {
        setConnection(connection);
    }

    // force logon if the server isn't open
    RuntimeConfig* runtimeConfig = _app->runtimeConfig();
    if (!user.loggedOn() && runtimeConfig->logonPolicy() != RuntimeConfig::Everyone) {
        showLogonDialog();
        return;
    }

    // continue into an instance
    if (transfer.instanceId != 0) {
        continueMovingToZone(transfer.sceneId, transfer.portal, peer, transfer.instanceId);

    } else if (user.lastZoneId != 0) {
        moveToZone(user.lastZoneId, user.lastSceneId);

    } else if (runtimeConfig->introZone() != 0) {
        moveToZone(runtimeConfig->introZone(), runtimeConfig->introScene());
    }
}

void Session::maybeSetConnection (
    const SharedConnectionPointer& connection, const QByteArray& token, const Callback& callback)
{
    if (_connection || _user.sessionToken != token) {
        callback.invoke(Q_ARG(bool, false));
        return;
    }
    setConnection(connection);
    callback.invoke(Q_ARG(bool, true));
}

QString Session::who () const
{
    return _user.name;
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

Evaluator* Session::evaluator ()
{
    if (_evaluator == 0) {
        _evaluator = new Evaluator(QString(), _chatEntryWindow->device(),
            _chatWindow->device(), _chatWindow->device(), this);
        connect(_evaluator, SIGNAL(exited(ScriptObjectPointer)),
            SLOT(evaluatorExited(ScriptObjectPointer)));
        connect(_evaluator, SIGNAL(threwError(ScriptError)),
            SLOT(evaluatorThrewError(ScriptError)));
    }
    return _evaluator;
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
    window->addChild(BoxLayout::createHBox(Qt::AlignCenter, 2, button));
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

void Session::loggedOn (const UserRecord& user, bool stayLoggedIn)
{
    _user = user;

    qDebug() << "Logged on." << user.id << user.name;
    
    if (_connection) {
        // set the cookies on the client
        Connection::setCookieMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, "username"), Q_ARG(const QString&, user.name));
        Connection::setCookieMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, "stay_logged_in"),
            Q_ARG(const QString&, stayLoggedIn ? "true" : "false"));
    }

    // update mappings
    userChanged();
}

void Session::logoff ()
{
    // request a new, anonymous user from the repository
    QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "createUser",
        Q_ARG(const Callback&, Callback(_this, "loggedOff(UserRecord)")));
}

void Session::moveToScene (const QString& prefix)
{
    // look up the prefix in the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "findScenes",
        Q_ARG(const QString&, prefix), Q_ARG(quint32, 0), Q_ARG(const Callback&,
            Callback(_this, "continueMovingToScene(ResourceDescriptorList)")));
}

void Session::moveToScene (quint32 id, const QVariant& portal)
{
    // resolve the scene via the instance
    if (_instance == 0) {
        showInfoDialog(tr("Not in an instance."));
        return;
    }
    _instance->resolveScene(id, Callback(_this, "sceneMaybeResolved(QVariant,QObject*)",
        Q_ARG(const QVariant&, portal)));
}

void Session::moveToZone (const QString& prefix)
{
    // look up the prefix in the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "findZones",
        Q_ARG(const QString&, prefix), Q_ARG(quint32, 0), Q_ARG(const Callback&,
            Callback(_this, "continueMovingToZone(ResourceDescriptorList)")));
}

void Session::moveToZone (quint32 id, quint32 sceneId, const QVariant& portal)
{
    // if we're already in the zone, move to the scene
    if (_instance != 0 && _instance->record().id == id) {
        moveToScene(sceneId, portal);
        return;
    }

    // reserve a place in an instance
    QMetaObject::invokeMethod(_app->peerManager(), "reserveInstancePlace",
        Q_ARG(quint64, _user.id), Q_ARG(const QString&, _region), Q_ARG(quint32, id),
        Q_ARG(const Callback&, Callback(_this,
            "continueMovingToZone(quint32,QVariant,QString,quint64)",
            Q_ARG(quint32, sceneId), Q_ARG(const QVariant&, portal))));
}

void Session::moveToInstance (quint64 id, quint32 sceneId, const QVariant& portal)
{
    // if we're already in the instance, move to the scene
    if (_instance != 0 && _instance->info().id == id) {
        moveToScene(sceneId, portal);
        return;
    }

    // reserve a place in an instance
    QMetaObject::invokeMethod(_app->peerManager(), "reserveInstancePlace",
        Q_ARG(quint64, _user.id), Q_ARG(quint64, id),
        Q_ARG(const Callback&, Callback(_this,
            "continueMovingToZone(quint32,QVariant,QString,quint64)",
            Q_ARG(quint32, sceneId), Q_ARG(const QVariant&, portal))));
}

void Session::moveToPlayer (const QString& name)
{
    QMetaObject::invokeMethod(_app->peerManager(), "getSessionInfo", Q_ARG(const QString&, name),
        Q_ARG(const Callback&, Callback(_this, "continueMovingToPlayer(QString,SessionInfo)",
            Q_ARG(const QString&, name))));
}

void Session::summonPlayer (const QString& name)
{
    _app->peerManager()->invokeSession(name, _app->connectionManager(),
        "summon(QString,QString,Callback)", Q_ARG(const QString&, name),
        Q_ARG(const QString&, _user.name), Q_ARG(const Callback&, Callback(
            _this, "maybeSummoned(QString,bool)", Q_ARG(const QString&, name))));
}

void Session::spawnActor (const QString& prefix)
{
    // look up the prefix in the database
    QMetaObject::invokeMethod(_app->databaseThread()->actorRepository(), "findActors",
        Q_ARG(const QString&, prefix), Q_ARG(quint32, 0), Q_ARG(const Callback&,
            Callback(_this, "continueSpawningActor(ResourceDescriptorList)")));
}

void Session::reconnect (const QString& host, quint16 portOffset)
{
    if (_connection) {
        Connection::reconnectMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, host), Q_ARG(quint16, portOffset));
    }
    close();
}

void Session::setSettings (const QString& password, const QString& email, QChar avatar)
{
    // set and persist in user record
    _user.setPassword(password);
    _user.email = email;
    _user.avatar = avatar;
    QMetaObject::invokeMethod(_app->databaseThread()->userRepository(), "updateUser",
        Q_ARG(const UserRecord&, _user), Q_ARG(const Callback&, Callback()));
}

void Session::say (const QString& message, ChatWindow::SpeakMode mode)
{
    if (mode == ChatWindow::BroadcastMode) {
        _app->peerManager()->invoke(_app->connectionManager(), "broadcast(QString,QString)",
            Q_ARG(const QString&, _user.name), Q_ARG(const QString&, message));
        return;
    }
    if (_scene != 0 && _pawn != 0) {
        // replace double with single quotes to prevent spoofing
        QString msg = message;
        msg.replace('\"', '\'');
        _scene->say(_pawn->position(), _user.name, msg, mode);
    }
}

void Session::tell (const QString& recipient, const QString& message)
{
    _app->peerManager()->invokeSession(recipient, _app->connectionManager(), "tell",
        Q_ARG(const QString&, _user.name), Q_ARG(const QString&, message),
        Q_ARG(const QString&, recipient), Q_ARG(const Callback&, Callback(_this,
            "maybeTold(QString,QString,bool)", Q_ARG(const QString&, recipient),
            Q_ARG(const QString&, message))));
}

void Session::mute (const QString& username)
{
}

void Session::unmute (const QString& username)
{
}

void Session::submitBugReport (const QString& description)
{
    qDebug() << "Submitting bug report." << who() << description;
    _app->sendMail(_app->bugReportAddress(), "Witgap bug report",
        who() + ": " + description, Callback());
}

void Session::openUrl (const QUrl& url)
{
    if (_connection) {
        Connection::evaluateMetaMethod().invoke(_connection.data(), Q_ARG(const QString&,
            "document.getElementById('importExport').src = '" + url.toString() + "'"));
    }
}

void Session::runScript (const QByteArray& script)
{
    evaluator()->evaluate(script);
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

void Session::showLogonDialog ()
{
    if (_connection) {
        new LogonDialog(this, _connection->cookies().value("username"),
            _connection->cookies().value("stay_logged_in") == "true");
    } else {
        new LogonDialog(this, "", false);
    }
}

void Session::showLogoffDialog ()
{
    showConfirmDialog(tr("Are you sure you want to log off?"), Callback(_this, "logoff()"));
}

void Session::showGoToZoneDialog ()
{
    ZoneChooserDialog* dialog = new ZoneChooserDialog(this, 0, false);
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(moveToZone(ResourceDescriptor)));
}

void Session::showGoToSceneDialog ()
{
    SceneChooserDialog* dialog = new SceneChooserDialog(this, 0, false);
    connect(dialog, SIGNAL(resourceChosen(ResourceDescriptor)),
        SLOT(moveToScene(ResourceDescriptor)));
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

    // start the close timer
    _closeTimer->start(DisconnectTimeout);

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

void Session::shutdown ()
{
    close();
}

void Session::close ()
{
    leaveScene();

    // let the connection manager update its mappings
    QMetaObject::invokeMethod(_app->connectionManager(), "sessionClosed",
        Q_ARG(quint64, _user.id), Q_ARG(const QString&, _user.name));

    // remove info on all peers
    _app->peerManager()->invoke(_app->peerManager(), "sessionRemoved(quint64,QString)",
        Q_ARG(quint64, _user.id), Q_ARG(const QString&, _info.peer));

    qDebug() << "Session closed." << _user.id << _user.name;
}

void Session::evaluatorExited (const ScriptObjectPointer& result)
{
    _chatWindow->display(result->toString());
}

void Session::evaluatorThrewError (const ScriptError& error)
{
    _chatWindow->display(error.toString());
}

void Session::setConnection (const SharedConnectionPointer& connection)
{
    // make sure it hasn't already been closed
    Connection* conn = connection.data();
    connect(conn, SIGNAL(closed()), SLOT(clearConnection()));
    if (!conn->isOpen()) {
        return;
    }
    _closeTimer->stop();
    _connection = connection;

    _displaySize = conn->displaySize();
    _region = conn->region();
    connect(conn, SIGNAL(windowClosed()), SLOT(shutdown()));
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
    loggedOn(urec, false);

    // open the settings dialog in order to change the password
    new SettingsDialog(this);
}

void Session::sceneCreated (quint32 id)
{
    qDebug() << "Created scene." << _user.name << id;

    moveToScene(id);
}

void Session::zoneCreated (quint32 id)
{
    qDebug() << "Created zone." << _user.name << id;

    moveToZone(id);
}

void Session::continueMovingToScene (const ResourceDescriptorList& scenes)
{
    if (scenes.isEmpty()) {
        _chatWindow->display(tr("No such scene."));
        return;
    }
    // pick one at random
    moveToScene(scenes.at(qrand() % scenes.size()).id);
}

void Session::sceneMaybeResolved (const QVariant& portal, QObject* scene)
{
    if (scene == 0) {
        showInfoDialog(tr("Failed to resolve scene."));
        return;
    }
    // enter the scene
    _scene = static_cast<Scene*>(scene);
    _mainWindow->connect(_scene, SIGNAL(recordChanged(SceneRecord)), SLOT(updateTitle()));
    _pawn = _scene->addSession(this, portal);
    _mainWindow->updateTitle();

    // update info on all peers
    _info.sceneId = _scene->record().id;
    _app->peerManager()->invoke(_app->peerManager(), "sessionUpdated(SessionInfo)",
        Q_ARG(const SessionInfo&, _info));

    emit didEnterScene(_scene);
}

void Session::leaveScene ()
{
    if (_scene != 0) {
        emit willLeaveScene(_scene);

        _scene->removeSession(this);
        _scene->disconnect(_mainWindow);
        _scene = 0;
        _pawn = 0;

        // update info on all peers
        _info.sceneId = 0;
        _app->peerManager()->invoke(_app->peerManager(), "sessionUpdated(SessionInfo)",
            Q_ARG(const SessionInfo&, _info));
    }
}

void Session::continueMovingToZone (const ResourceDescriptorList& zones)
{
    if (zones.isEmpty()) {
        _chatWindow->display(tr("No such zone."));
        return;
    }
    // pick one at random
    moveToZone(zones.at(qrand() % zones.size()).id);
}

void Session::continueMovingToPlayer (const QString& name, const SessionInfo& info)
{
    if (info.id == 0) {
        _chatWindow->display(tr("There is no one online named %1.").arg(name));
        return;
    }
    if (info.instanceId == 0) {
        _chatWindow->display(tr("%1 is not in an instance.").arg(name));
        return;
    }
    moveToInstance(info.instanceId, info.sceneId, name);
}

void Session::continueMovingToZone (
    quint32 sceneId, const QVariant& portal, const QString& peer, quint64 instanceId)
{
    // if it's not the local peer, we need to initiate a session transfer
    if (peer != _app->peerManager()->record().name) {
        qDebug() << "Transferring session." << who() << peer;
        SessionTransfer transfer;
        transfer.user = _user;
        transfer.instanceId = instanceId;
        transfer.sceneId = sceneId;
        transfer.portal = portal;
        transfer.chatCommandHistory = _chatEntryWindow->history();
        _app->peerManager()->invoke(peer, _app->connectionManager(),
            "transferSession(SessionTransfer)", Q_ARG(const SessionTransfer&, transfer));
        return;
    }
    // move the session to the instance thread
    leaveZone();
    setParent(0);
    Instance* instance = _app->sceneManager()->instance(instanceId);
    moveToThread(instance->thread());

    // continue the process in the instance thread
    QMetaObject::invokeMethod(this, "continueMovingToZone", Q_ARG(QObject*, instance),
        Q_ARG(quint32, sceneId), Q_ARG(const QVariant&, portal));
}

void Session::continueSpawningActor (const ResourceDescriptorList& actors)
{
    if (actors.isEmpty()) {
        _chatWindow->display(tr("No such actor."));
        return;
    }
    // pick one at random
    // spawnActor(actors.at(qrand() % actors.size()).id);
}

void Session::leaveZone ()
{
    if (_instance != 0) {
        _instance->removeSession(this);
        _instance->disconnect(_mainWindow);
        _instance = 0;

        // update info on all peers
        _info.instanceId = 0;
        _app->peerManager()->invoke(_app->peerManager(), "sessionUpdated(SessionInfo)",
            Q_ARG(const SessionInfo&, _info));
    }
}

void Session::continueMovingToZone (QObject* instance, quint32 sceneId, const QVariant& portal)
{
    _instance = static_cast<Instance*>(instance);
    _mainWindow->connect(_instance, SIGNAL(recordChanged(ZoneRecord)), SLOT(updateTitle()));
    _instance->addSession(this);
    _mainWindow->updateTitle();

    // update info on all peers
    _info.instanceId = 0;
    _app->peerManager()->invoke(_app->peerManager(), "sessionUpdated(SessionInfo)",
        Q_ARG(const SessionInfo&, _info));

    // now move to the requested (or default) scene
    if (sceneId == 0) {
        sceneId = _instance->record().defaultSceneId;
    }
    if (sceneId != 0) {
        moveToScene(sceneId, portal);
    }
}

void Session::maybeTold (const QString& recipient, const QString& message, bool success)
{
    _chatWindow->display(success ? tr("You tell %1, \"%2\"").arg(recipient, message) :
        tr("There is no one online named %1.").arg(recipient));
}

void Session::maybeSummoned (const QString& name, bool success)
{
    if (!success) {
        _chatWindow->display(tr("There is no one online named %1.").arg(name));
    }
}

void Session::loggedOff (const UserRecord& user)
{
    qDebug() << "Logged off." << _user.id << _user.name;
   
    _user = user;
    
    // update mappings
    userChanged();

    // force logon if the server isn't open
    if (_app->runtimeConfig()->logonPolicy() != RuntimeConfig::Everyone) {
        showLogonDialog();
    }
}

void Session::userChanged ()
{
    // let the connection manager update its mappings
    QMetaObject::invokeMethod(_app->connectionManager(), "sessionChanged",
        Q_ARG(quint64, _info.id), Q_ARG(quint64, _user.id),
        Q_ARG(const QString&, _info.name), Q_ARG(const QString&, _user.name));

    // update the info on all peers
    _app->peerManager()->invoke(_app->peerManager(), "sessionRemoved(quint64,QString)",
        Q_ARG(quint64, _info.id), Q_ARG(const QString&, _info.peer));
    _info.id = _user.id;
    _info.name = _user.name;
    _app->peerManager()->invoke(_app->peerManager(), "sessionAdded(SessionInfo)",
        Q_ARG(const SessionInfo&, _info));

    if (_connection) {
        // set the cookies on the client
        Connection::setCookieMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, "userId"), Q_ARG(const QString&,
                QString::number(_user.id, 16).rightJustified(16, '0')));
        Connection::setCookieMetaMethod().invoke(_connection.data(),
            Q_ARG(const QString&, "sessionToken"), Q_ARG(const QString&,
                _user.sessionToken.toHex()));
    }

    // update the window title
    _mainWindow->updateTitle();
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
