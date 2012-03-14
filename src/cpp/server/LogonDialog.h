//
// $Id$

#ifndef LOGON_DIALOG
#define LOGON_DIALOG

#include "ui/Window.h"

class Button;
class Label;
class PasswordField;
class ServerApp;
class Session;
class StatusLabel;
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
    LogonDialog (ServerApp* app, Session* parent, const QString& username);

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
     * If the id is non-zero, the user was inserted with that id.
     */
    Q_INVOKABLE void userMaybeInserted (quint32 id);

    /**
     * If the result contains a UserRecord, the user was logged on; otherwise, the result
     * will contain an error code.
     */
    Q_INVOKABLE void logonMaybeValidated (const QVariant& result);

    /**
     * Switches between account creation and logon mode.
     */
    void setCreateMode (bool createMode);

    /**
     * Flashes the specified status message.
     */
    void flashStatus (const QString& status);

    /** The server application. */
    ServerApp* _app;

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

    /** The status label. */
    StatusLabel* _status;

    /** The create mode toggle button. */
    Button* _toggleCreateMode;

    /** The cancel and logon buttons. */
    Button* _cancel, *_logon;

    /** If true, we're blocking logon (for a database query, e.g.) */
    bool _logonBlocked;
};

#endif // LOGON_DIALOG
