//
// $Id$

#ifndef EDIT_USER_DIALOG
#define EDIT_USER_DIALOG

#include "ui/Window.h"

class Button;
class CheckBox;
class Label;
class PasswordField;
class ServerApp;
class Session;
class StatusLabel;
class TextField;
class UserRecord;

/**
 * Allows admins to edit/delete user accounts.
 */
class EditUserDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    EditUserDialog (Session* parent);

protected slots:

    /**
     * Updates the state of the search button.
     */
    void updateSearch ();

    /**
     * Searches for the named user.
     */
    void search ();

    /**
     * Makes sure the admin really wants to delete the user.
     */
    void confirmDelete ();

    /**
     * Updates the user record.
     */
    void update ();

protected:

    /**
     * Responds to a request to load the user.
     */
    Q_INVOKABLE void userMaybeLoaded (const UserRecord& user);

    /**
     * Actually deletes the user, having confirmed that that's what the admin wants.
     */
    Q_INVOKABLE void reallyDelete ();

    /** The current username field. */
    TextField* _username;

    /** The search button. */
    Button* _search;

    /** The id label. */
    Label* _id;

    /** The creation time label. */
    Label* _created;

    /** The last online time label. */
    Label* _lastOnline;

    /** The new username field. */
    TextField* _newUsername;

    /** The password field. */
    PasswordField* _password;

    /** The password confirmation field. */
    PasswordField* _confirmPassword;

    /** The date of birth field. */
    TextField* _dob;

    /** The email field. */
    TextField* _email;

    /** The banned flag check box. */
    CheckBox* _banned;

    /** The admin flag check box. */
    CheckBox* _admin;

    /** The status label. */
    StatusLabel* _status;

    /** The update user button. */
    Button* _update;

    /** The delete user button. */
    Button* _delete;
};

#endif // EDIT_USER_DIALOG
