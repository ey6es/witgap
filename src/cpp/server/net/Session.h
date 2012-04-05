//
// $Id$

#ifndef SESSION
#define SESSION

#include <QSet>
#include <QSize>

#include "db/SessionRepository.h"
#include "db/UserRepository.h"
#include "ui/TextField.h"
#include "util/Callback.h"

class QEvent;
class QTranslator;

class Connection;
class MainWindow;
class Pawn;
class Scene;
class SceneRecord;
class ServerApp;
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
    Session (ServerApp* app, Connection* connection,
        const SessionRecord& record, const UserRecord& user);

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
    Connection* connection () const { return _connection; }

    /**
     * Replaces the session connection.
     */
    void setConnection (Connection* connection);

    /**
     * Returns a string identifying the user.
     */
    QString who () const;

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
     * Shows the logon dialog.
     */
    void showLogonDialog ();

    /**
     * Shows the logoff confirmation dialog.
     */
    void showLogoffDialog ();

    /**
     * Notes that the specified user has logged on.
     */
    void loggedOn (const UserRecord& user);

    /**
     * Logs off the current user.
     */
    Q_INVOKABLE void logoff ();

    /**
     * Initiates the process of creating a new scene and transferring the user to it.
     */
    void createScene ();

    /**
     * Moves the user to the identified scene.
     */
    void moveToScene (quint32 id);

    /**
     * Sets the user's settings.
     */
    void setSettings (const QString& password, const QString& email, QChar avatar);

    /**
     * Translates a string using the user's preferred language.
     */
    QString translate (
        const char* context, const char* sourceText, const char* disambiguation = 0, int n = -1);

    /**
     * Handles an event.
     */
    virtual bool event (QEvent* e);

protected slots:

    /**
     * Clears the connection pointer (because it has been destroyed).
     */
    void clearConnection ();

    /**
     * Clears the moused component (because it has been destroyed).
     */
    void clearMoused ();

    /**
     * Clears the active window (because it has been destroyed).
     */
    void clearActiveWindow ();

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

protected:

    /**
     * Reports back with the result of a password reset validation attempt.
     */
    Q_INVOKABLE void passwordResetMaybeValidated (const QVariant& result);

    /**
     * Submits a bug report with the supplied description.
     */
    Q_INVOKABLE void submitBugReport (const QString& description);

    /**
     * Reports back with id of the newly created scene.
     */
    Q_INVOKABLE void sceneCreated (quint32 id);

    /**
     * Reports back with the resolved scene object, if successful.
     */
    Q_INVOKABLE void sceneMaybeResolved (QObject* scene);

    /**
     * Enters the now-resolved scene.  This is called after the session has been transferred
     * to the scene thread.
     */
    Q_INVOKABLE void continueMovingToScene (QObject* scene);

    /**
     * Determines whether the specified window is beneath another, modal window.
     */
    bool belowModal (Window* window) const;

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    Connection* _connection;

    /** The session record. */
    SessionRecord _record;

    /** The last window id assigned. */
    int _lastWindowId;

    /** The size of the user's display. */
    QSize _displaySize;

    /** The number of elements requiring encryption.  When this drops to zero, we can disable. */
    int _cryptoCount;

    /** The main client window. */
    MainWindow* _mainWindow;

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

    /** The currently occupied scene. */
    Scene* _scene;

    /** The currently controlled pawn. */
    Pawn* _pawn;
};

#endif // SESSION
