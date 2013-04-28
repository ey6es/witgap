//
// $Id$

#ifndef GENERATE_INVITES_DIALOG
#define GENERATE_INVITES_DIALOG

#include "ui/Window.h"

class Button;
class CheckBox;
class Session;
class TextField;

/**
 * Allows admins to edit/delete user accounts.
 */
class GenerateInvitesDialog : public Window
{
    Q_OBJECT

public:
    
    /**
     * Initializes the dialog.
     */
    Q_INVOKABLE GenerateInvitesDialog (Session* parent);

protected slots:
   
    /**
     * Generates the invites.
     */
    void generate (); 
   
protected:
    
    /** The description field. */
    TextField* _description;
    
    /** The count field. */
    TextField* _count;
    
    /** The flag check boxes. */
    CheckBox* _admin, *_insider;
    
    /** The OK button. */
    Button* _ok;
};

#endif // GENERATE_INVITES_DIALOG
