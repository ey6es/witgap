//
// $Id$

#ifndef SESSION
#define SESSION

#include <QObject>
#include <QSize>

#include "ui/Window.h"

class Callback;
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
    Session (ServerApp* app, Connection* connection, quint64 id, const QByteArray& token);

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

    /** The component over which the mouse button was pressed. */
    Component* _moused;

    /** The component with input focus. */
    Component* _focus;
};

class Button;
class Label;
class PasswordField;
class TextField;

/**
 * Handles logging on or creating an account.
 */
class LogonDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     *
     * @param username the username cookie, if any.
     */
    LogonDialog (Session* parent, const QString& username);

protected slots:

    /**
     * Updates the state of the logon/create button based on the input.
     */
    void updateLogon ();

    /**
     * Toggles the mode between create and logon.
     */
    void toggleCreateMode () { setCreateMode(!_createMode); };

    /**
     * Attempts to log on using the current information.
     */
    void logon ();

protected:

    /**
     * Switches between account creation and logon mode.
     */
    void setCreateMode (bool createMode);

    /** Whether or not we're currently in create mode. */
    bool _createMode;

    /** The instruction label. */
    Label* _label;

    /** The username field. */
    TextField* _username;

    /** The password field. */
    PasswordField* _password;

    /** The confirm password field. */
    PasswordField* _confirmPassword;

    /** The month, day, and year fields. */
    TextField* _month, *_day, *_year;

    /** The email field. */
    TextField* _email;

    /** The create mode toggle button. */
    Button* _toggleCreateMode;

    /** The logon button. */
    Button* _logon;
};

#endif // SESSION
