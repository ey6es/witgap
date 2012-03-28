//
// $Id$

#ifndef PREFERENCES_DIALOG
#define PREFERENCES_DIALOG

#include "ui/Window.h"

class Button;
class TextField;

/**
 * Allows the user to change his preferences.
 */
class PreferencesDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    PreferencesDialog (Session* parent);

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

    /** The avatar field. */
    TextField* _avatar;

    /** The apply and OK buttons. */
    Button* _apply, *_ok;
};

#endif // PREFERENCES_DIALOG
