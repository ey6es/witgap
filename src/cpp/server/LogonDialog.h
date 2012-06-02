//
// $Id$

#ifndef LOGON_DIALOG
#define LOGON_DIALOG

#include <QRegExp>

#include "ui/Window.h"

class Button;
class Label;
class PasswordField;
class Session;
class StatusLabel;
class TextField;
class UserRecord;

/**
 * Handles logging on or creating an account.
 */
class LogonDialog : public EncryptedWindow
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     *
     * @param username the username cookie, if any.
     * @param force if true, force logon (because the server is closed to guests).
     * @param allowCreate whether or not to allow account creation.
     */
    LogonDialog (
        Session* parent, const QString& username, bool force = false, bool allowCreate = true);

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

    /**
     * Pops up a dialog to send the username in an email.
     */
    void forgotUsername ();

    /**
     * Pops up a dialog to send a password reset link in an email.
     */
    void forgotPassword ();

protected:

    /**
     * If the record id is non-zero, the user was inserted with that record.
     */
    Q_INVOKABLE void userMaybeInserted (const UserRecord& user);

    /**
     * If the result contains a UserRecord, the user was logged on; otherwise, the result
     * will contain an error code.
     */
    Q_INVOKABLE void logonMaybeValidated (const QVariant& result);

    /**
     * Sends a username email if the provided address matches one in the database.
     */
    Q_INVOKABLE void maybeSendUsernameEmail (const QString& email);

    /**
     * Sends a username email if the provided address matches one in the database.
     */
    Q_INVOKABLE void maybeSendUsernameEmail (const QString& email, const QString& username);

    /**
     * Sends a password email if the provided username matches one in the database.
     */
    Q_INVOKABLE void maybeSendPasswordEmail (const QString& username);

    /**
     * Sends a password email if the user record is valid and contains an email address.
     */
    Q_INVOKABLE void maybeSendPasswordEmail (const UserRecord& urec);

    /**
     * Sends the password email.
     */
    Q_INVOKABLE void sendPasswordEmail (
        const QString& email, quint32 resetId, const QByteArray& resetToken);

    /**
     * Called to report the result of attempting to send the email.
     */
    Q_INVOKABLE void emailMaybeSent (const QString& email, const QString& result);

    /**
     * Switches between account creation and logon mode.
     */
    void setCreateMode (bool createMode);

    /**
     * Flashes the specified status message.
     */
    void flashStatus (const QString& status);

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

    /** The forgot username/password buttons. */
    Button* _forgotUsername, *_forgotPassword;

    /** If true, we're blocking logon (for a database query, e.g.) */
    bool _logonBlocked;
};

/** Expressions for partial and complete usernames. */
const QRegExp PartialUsernameExp("[a-zA-Z0-9]{0,16}"), FullUsernameExp("[a-zA-Z0-9]{3,16}");

/** The maximum allowed username length. */
const int MaxUsernameLength = 16;

/** Expressions for partial and complete passwords. */
const QRegExp PartialPasswordExp(".{0,255}"), FullPasswordExp(".{6,255}");

/** Month/day and year expressions. */
const QRegExp MonthDayExp("\\d{0,2}"), YearExp("\\d{0,4}");

/** Partial and full email expressions (full from
 * http://www.regular-expressions.info/regexbuddy/email.html). */
const QRegExp PartialEmailExp("[a-zA-Z0-9._%-]*@?[a-zA-Z0-9.-]*\\.?[a-zA-Z]{0,4}");
const QRegExp FullEmailExp("[a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}");

#endif // LOGON_DIALOG
