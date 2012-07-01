//
// $Id$

#ifndef SESSION
#define SESSION

#include <QList>
#include <QSet>
#include <QSize>

#include "chat/ChatWindow.h"
#include "db/SessionRepository.h"
#include "db/UserRepository.h"
#include "net/Connection.h"
#include "peer/PeerManager.h"
#include "ui/TextField.h"
#include "util/Callback.h"
#include "util/General.h"
#include "util/Streaming.h"

class QEvent;
class QTimer;
class QTranslator;

class ChatEntryWindow;
class Instance;
class MainWindow;
class Pawn;
class Scene;
class ServerApp;
class SessionTransfer;
class Window;

/**
 * Handles a single user session.
 */
class Session : public CallableObject
{
    Q_OBJECT

public:

    /**
     * Initializes the session.
     */
    Session (ServerApp* app, const SharedConnectionPointer& connection,
        const SessionRecord& record, const UserRecord& user, const SessionTransfer& transfer);

    /**
     * Returns a pointer to the application object.
     */
    ServerApp* app () const { return _app; }

    /**
     * Returns a reference to the session record.
     */
    const SessionRecord& record () const { return _record; }

    /**
     * Returns a pointer to the connection, or zero if unconnected.
     */
    Connection* connection () const { return _connection.data(); }

    /**
     * Attempts to set the connection.  The callback will receive a bool indicating whether the
     * connection was set.
     */
    Q_INVOKABLE void maybeSetConnection (
        const SharedConnectionPointer& connection, const QByteArray& token,
        const Callback& callback);

    /**
     * Returns a string identifying the user.
     */
    QString who () const;

    /**
     * Returns the supported locale of the session.
     */
    QString locale () const;

    /**
     * Checks whether the session is associated with a logged-on user.
     */
    bool loggedOn () const { return _user.id != 0; }

    /**
     * Checks whether the session is associated with an admin.
     */
    bool admin () const { return _user.id != 0 && _user.flags.testFlag(UserRecord::Admin); }

    /**
     * Returns a reference to the user record.
     */
    const UserRecord& user () const { return _user; }

    /**
     * Returns the instance pointer, if the user is in a zone instance.
     */
    Instance* instance () const { return _instance; }

    /**
     * Returns the scene pointer, if the user is in a scene.
     */
    Scene* scene () const { return _scene; }

    /**
     * Returns the pawn pointer, if the user has a pawn in the scene.
     */
    Pawn* pawn () const { return _pawn; }

    /**
     * Increments the window id counter and returns its value.
     */
    int nextWindowId () { return ++_lastWindowId; }

    /**
     * Finds and returns the highest layer among all added windows, or zero if there aren't any
     * windows.
     */
    int highestWindowLayer () const;

    /**
     * Returns the size of the user's display.
     */
    const QSize& displaySize () const { return _displaySize; }

    /**
     * Increments the number of elements requiring encryption.  If it was previously zero,
     * encryption will be enabled.
     */
    void incrementCryptoCount ();

    /**
     * Decrements the number of elements requiring encryption.  When it reaches zero,
     * encryption will be disabled.
     */
    void decrementCryptoCount ();

    /**
     * Returns a pointer to the chat window.
     */
    ChatWindow* chatWindow () const { return _chatWindow; }

    /**
     * Returns a pointer to the chat entry window.
     */
    ChatEntryWindow* chatEntryWindow () const { return _chatEntryWindow; }

    /**
     * Sets the active window.
     */
    void setActiveWindow (Window* window);

    /**
     * Returns a pointer to the active window.
     */
    Window* activeWindow () const { return _activeWindow; }

    /**
     * Updates the active window according to the current window states.
     */
    void updateActiveWindow ();

    /**
     * Returns a pointer to the translator used by the session.
     */
    QTranslator* translator () const { return _translator; }

    /**
     * Requests focus for the specified component.
     */
    void requestFocus (Component* component);

    /**
     * Shows a simple info dialog with the supplied message.
     *
     * @param title the title to use for the dialog, or empty string for none.
     * @param dismiss the text to use for the dismiss button, or empty string for default (OK).
     */
    void showInfoDialog (
        const QString& message, const QString& title = "", const QString& dismiss = "");

    /**
     * Shows a simple confirmation dialog with the supplied message.
     *
     * @param callback the callback to invoke if accepted.
     * @param title the title to use for the dialog, or empty string for none.
     * @param dismiss the text to use for the dismiss button, or empty string for default (Cancel).
     * @param accept the text to use for the accept button, or empty string for default (OK).
     */
    void showConfirmDialog (
        const QString& message, const Callback& callback, const QString& title = "",
        const QString& dismiss = "", const QString& accept = "");

    /**
     * Shows a simple input dialog with the supplied message.
     *
     * @param callback the callback to invoke with the input string if accepted.
     * @param title the title to use for the dialog, or empty string for none.
     * @param dismiss the text to use for the dismiss button, or empty string for default (Cancel).
     * @param accept the text to use for the accept button, or empty string for default (OK).
     * @param acceptExp the regular expression that, when matches, enables the accept button.
     */
    void showInputDialog (
        const QString& message, const Callback& callback, const QString& title = "",
        const QString& dismiss = "", const QString& accept = "",
        Document* document = new Document(), const QRegExp& acceptExp = NonEmptyExp);

    /**
     * Notes that the specified user has logged on.
     */
    void loggedOn (const UserRecord& user);

    /**
     * Logs off the current user.
     */
    Q_INVOKABLE void logoff ();

    /**
     * Moves the user to the named scene.
     */
    void moveToScene (const QString& prefix);

    /**
     * Moves the user to the identified scene.
     */
    void moveToScene (quint32 id, const QVariant& portal = QVariant());

    /**
     * Moves the user to the named zone.
     */
    void moveToZone (const QString& prefix);

    /**
     * Moves the user to the identified zone.
     */
    void moveToZone (quint32 id, quint32 sceneId = 0, const QVariant& portal = QVariant());

    /**
     * Moves to the named player.
     */
    void moveToPlayer (const QString& name);

    /**
     * Summons the named player.
     */
    void summonPlayer (const QString& name);

    /**
     * Closes the session, instructing it to reconnect to the specified host and port.
     */
    Q_INVOKABLE void reconnect (const QString& host, quint16 port);

    /**
     * Sets the user's settings.
     */
    void setSettings (const QString& password, const QString& email, QChar avatar);

    /**
     * Speaks a message in the current place.
     */
    void say (const QString& message, ChatWindow::SpeakMode mode = ChatWindow::NormalMode);

    /**
     * Attempts to send a tell to the named recipient.
     */
    void tell (const QString& recipient, const QString& message);

    /**
     * Submits a bug report with the supplied description.
     */
    Q_INVOKABLE void submitBugReport (const QString& description);

    /**
     * Notifies us that a window has been created.
     */
    void windowCreated (Window* window);

    /**
     * Notifies us that a window has been destroyed.
     */
    void windowDestroyed (Window* window);

    /**
     * Handles an event.
     */
    virtual bool event (QEvent* e);

signals:

    /**
     * Fired when the session has entered a scene.
     */
    void didEnterScene (Scene* scene);

    /**
     * Fired just before the session leaves a scene.
     */
    void willLeaveScene (Scene* scene);

public slots:

    /**
     * Shows the logon dialog.
     */
    void showLogonDialog ();

    /**
     * Shows the logoff confirmation dialog.
     */
    void showLogoffDialog ();

    /**
     * Initiates the process of creating a new scene and transferring the user to it.
     */
    void createScene ();

    /**
     * Initiates the process of creating a new zone and transferring the user to it.
     */
    void createZone ();

protected slots:

    /**
     * Clears the connection pointer (because it's going to be destroyed).
     */
    void clearConnection ();

    /**
     * Clears the moused component (because it has been destroyed).
     */
    void clearMoused ();

    /**
     * Dispatches a mouse pressed event.
     */
    void dispatchMousePressed (int x, int y);

    /**
     * Dispatches a mouse released event.
     */
    void dispatchMouseReleased (int x, int y);

    /**
     * Dispatches a key pressed event.
     */
    void dispatchKeyPressed (int key, QChar ch, bool numpad);

    /**
     * Dispatches a key released event.
     */
    void dispatchKeyReleased (int key, QChar ch, bool numpad);

    /**
     * Closes the session.
     */
    void close ();

protected:

    /**
     * Replaces the session connection.
     */
    void setConnection (const SharedConnectionPointer& connection);

    /**
     * Reports back with the result of a password reset validation attempt.
     */
    Q_INVOKABLE void passwordResetMaybeValidated (const QVariant& result);

    /**
     * Reports back with id of the newly created scene.
     */
    Q_INVOKABLE void sceneCreated (quint32 id);

    /**
     * Reports back with the id of the newly created zone.
     */
    Q_INVOKABLE void zoneCreated (quint32 id);

    /**
     * Continues the process of moving to a scene.
     */
    Q_INVOKABLE void continueMovingToScene (const DescriptorList& scenes);

    /**
     * Reports back with the resolved scene object, if successful.
     */
    Q_INVOKABLE void sceneMaybeResolved (const QVariant& portal, QObject* scene);

    /**
     * Continues the process of moving to a zone.
     */
    Q_INVOKABLE void continueMovingToZone (const DescriptorList& zones);

    /**
     * Continues the process of moving to a zone.
     */
    Q_INVOKABLE void continueMovingToZone (
        quint32 sceneId, const QVariant& portal, const QString& peer, quint64 instanceId);

    /**
     * Leaves the current scene, if any.
     */
    void leaveScene ();

    /**
     * Leaves the current zone, if any.
     */
    void leaveZone ();

    /**
     * Enters the instance in which a place has been reserved for us.  This is called after the
     * session has been transferred to the instance thread.
     */
    Q_INVOKABLE void continueMovingToZone (
        QObject* instance, quint32 sceneId, const QVariant& portal);

    /**
     * Reports back from a tell request.
     */
    Q_INVOKABLE void maybeTold (const QString& recipient, const QString& message, bool success);

    /**
     * Reports back from a logoff request.
     *
     * @param name the new session name.
     */
    Q_INVOKABLE void loggedOff (const QString& name);

    /**
     * Handles a name change.
     */
    void nameChanged (const QString& oldName);

    /**
     * Determines whether the specified window is beneath another, modal window.
     */
    bool belowModal (Window* window) const;

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    SharedConnectionPointer _connection;

    /** The timer that closes the session after extended disconnect. */
    QTimer* _closeTimer;

    /** The session record. */
    SessionRecord _record;

    /** The session info reported to peers. */
    SessionInfo _info;

    /** The last window id assigned. */
    int _lastWindowId;

    /** The size of the user's display. */
    QSize _displaySize;

    /** The user's closest region. */
    QString _region;

    /** The number of elements requiring encryption.  When this drops to zero, we can disable. */
    int _cryptoCount;

    /** The list of registered windows. */
    QList<Window*> _windows;

    /** The main client window. */
    MainWindow* _mainWindow;

    /** The chat display window. */
    ChatWindow* _chatWindow;

    /** The chat entry window. */
    ChatEntryWindow* _chatEntryWindow;

    /** The current set of key modifiers. */
    Qt::KeyboardModifiers _modifiers;

    /** The set of keys pressed. */
    QSet<int> _keysPressed;

    /** Whether or not the mouse button is pressed. */
    bool _mousePressed;

    /** The component over which the mouse button was pressed. */
    Component* _moused;

    /** The active window (the one that holds the input focus). */
    Window* _activeWindow;

    /** The translator for the user's language, if any. */
    QTranslator* _translator;

    /** The currently logged in user. */
    UserRecord _user;

    /** The currently occupied zone instance. */
    Instance* _instance;

    /** The currently occupied scene. */
    Scene* _scene;

    /** The currently controlled pawn. */
    Pawn* _pawn;
};

/**
 * Contains the session state sent between peers when a session is transferred.
 */
class SessionTransfer
{
    STREAMABLE

public:

    /** The session record. */
    STREAM SessionRecord record;

    /** The user record. */
    STREAM UserRecord user;

    /** The instance to enter after transfer, if any. */
    STREAM quint64 instanceId;

    /** The scene to enter after transfer, if any. */
    STREAM quint32 sceneId;

    /** The portal at which to enter the scene. */
    STREAM QVariant portal;
};

DECLARE_STREAMABLE_METATYPE(SessionTransfer)

#endif // SESSION
