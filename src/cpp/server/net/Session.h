//
// $Id$

#ifndef SESSION
#define SESSION

#include <QObject>
#include <QSet>
#include <QSize>

#include "db/UserRepository.h"

class QEvent;
class QTranslator;

class Callback;
class Component;
class Connection;
class ServerApp;

/**
 * Handles a single user session.
 */
class Session : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the session.
     */
    Session (ServerApp* app, Connection* connection, quint64 id,
        const QByteArray& token, const UserRecord& user);

    /**
     * Returns a pointer to the application object.
     */
    ServerApp* app () const { return _app; }

    /**
     * Returns the session id.
     */
    quint64 id () const { return _id; }

    /**
     * Returns the session token.
     */
    const QByteArray& token () const { return _token; }

    /**
     * Returns a pointer to the connection, or zero if unconnected.
     */
    Connection* connection () const { return _connection; }

    /**
     * Replaces the session connection.
     */
    void setConnection (Connection* connection);

    /**
     * Checks whether the session is associated with a logged-on user.
     */
    bool loggedOn () const { return _user.id != 0; }

    /**
     * Checks whether the session is associated with an admin.
     */
    bool admin () const { return _user.id != 0 && _user.flags.testFlag(UserRecord::Admin); }

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
     * Sets focus to the specified component.
     */
    void setFocus (Component* component);

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
     */
    void showInputDialog (
        const QString& message, const Callback& callback, const QString& title = "",
        const QString& dismiss = "", const QString& accept = "");

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
     * Clears the focused component (because it has been destroyed).
     */
    void clearFocus ();

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
     * Shows the logon dialog with the provided default username.
     */
    Q_INVOKABLE void showLogonDialog (const QString& username);

    /** The server application. */
    ServerApp* _app;

    /** The session connection. */
    Connection* _connection;

    /** The session id. */
    quint64 _id;

    /** The session token. */
    QByteArray _token;

    /** The last window id assigned. */
    int _lastWindowId;

    /** The size of the user's display. */
    QSize _displaySize;

    /** The current set of key modifiers. */
    Qt::KeyboardModifiers _modifiers;

    /** The set of keys pressed. */
    QSet<int> _keysPressed;

    /** Whether or not the mouse button is pressed. */
    bool _mousePressed;

    /** The component over which the mouse button was pressed. */
    Component* _moused;

    /** The component with input focus. */
    Component* _focus;

    /** The translator for the user's language, if any. */
    QTranslator* _translator;

    /** The currently logged in user. */
    UserRecord _user;
};

#endif // SESSION
