//
// $Id$

#ifndef ZONE_PROPERTIES_DIALOG
#define ZONE_PROPERTIES_DIALOG

#include "ui/Window.h"

class Button;
class Session;
class TextField;

/**
 * Allows zone owners and admins to edit the properties of the zone.
 */
class ZonePropertiesDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    Q_INVOKABLE ZonePropertiesDialog (Session* parent);

protected slots:

    /**
     * Updates the fields in response to an external change.
     */
    void update ();

    /**
     * Updates the state of the apply/OK buttons.
     */
    void updateApply ();

    /**
     * Makes sure the user really wants to delete the zone.
     */
    void confirmDelete ();

    /**
     * Applies the changes.
     */
    void apply ();

protected:

    /**
     * Actually deletes the zone, having confirmed that that's what the user wants.
     */
    Q_INVOKABLE void reallyDelete ();

    /** The name field. */
    TextField* _name;

    /** The max population field. */
    TextField* _maxPopulation;

    /** The apply/OK buttons. */
    Button* _apply, *_ok;
};

#endif // ZONE_PROPERTIES_DIALOG
