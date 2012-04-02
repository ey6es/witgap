//
// $Id$

#ifndef SETTINGS_DIALOG
#define SETTINGS_DIALOG

#include "ui/Window.h"

class Button;
class PasswordField;
class TextField;

/**
 * Allows the user to change his settings.
 */
class SettingsDialog : public EncryptedWindow
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    SettingsDialog (Session* parent);

protected slots:

    /**
     * Updates the state of the apply/OK buttons.
     */
    void updateApply ();

    /**
     * Applies the requested changes.
     */
    void apply ();

protected:

    /** The password field. */
    PasswordField* _password;

    /** The password confirmation field. */
    PasswordField* _confirmPassword;

    /** The email field. */
    TextField* _email;

    /** The avatar field. */
    TextField* _avatar;

    /** The apply and OK buttons. */
    Button* _apply, *_ok;
};

#endif // SETTINGS_DIALOG
