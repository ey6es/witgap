//
// $Id$

#ifndef EDIT_USER_DIALOG
#define EDIT_USER_DIALOG

#include "db/UserRepository.h"
#include "ui/Window.h"

class Button;
class CheckBox;
class Label;
class PasswordField;
class Session;
class StatusLabel;
class TextField;

/**
 * Allows admins to edit/delete user accounts.
 */
class EditUserDialog : public EncryptedWindow
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    Q_INVOKABLE EditUserDialog (Session* parent);

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
     * Updates the state of the apply/OK buttons.
     */
    void updateApply ();

    /**
     * Makes sure the admin really wants to delete the user.
     */
    void confirmDelete ();

    /**
     * Updates the user record.
     */
    void apply ();

protected:

    /**
     * Responds to a request to load the user.
     */
    Q_INVOKABLE void userMaybeLoaded (const UserRecord& user);

    /**
     * Actually deletes the user, having confirmed that that's what the admin wants.
     */
    Q_INVOKABLE void reallyDelete ();

    /**
     * Responds to a request to update the user.
     */
    Q_INVOKABLE void userMaybeUpdated (bool updated);

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

    /** The delete user button. */
    Button* _delete;

    /** The apply/OK buttons. */
    Button* _apply, *_ok;

    /** The loaded user record. */
    UserRecord _user;
};

#endif // EDIT_USER_DIALOG
