//
// $Id$

#ifndef GENERATE_INVITES_DIALOG
#define GENERATE_INVITES_DIALOG

#include "ui/Window.h"

class Button;
class CheckBox;
class Session;
class TabbedPane;
class TextArea;
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
   
    /**
     * Displays the invite URL.
     */
    Q_INVOKABLE void showInviteUrl (const QString& url);
    
    /**
     * Mails a set of invites.
     */
    Q_INVOKABLE void mailInvites (const QStringList& emails, const QStringList& urls);
    
    /**
     * Reports back from a single email request.
     */
    Q_INVOKABLE void emailMaybeSent (const QString& email, const QString& error);
    
    /** The description field. */
    TextField* _description;
    
    /** The tabs containing our bits. */
    TabbedPane* _tabs;
    
    /** The count field. */
    TextField* _count;
    
    /** The emails area. */
    TextArea* _emails;
    
    /** The flag check boxes. */
    CheckBox* _admin, *_insider;
    
    /** The OK button. */
    Button* _ok;
    
    /** The emails remaining to be sent. */
    int _emailsRemaining;
};

#endif // GENERATE_INVITES_DIALOG
