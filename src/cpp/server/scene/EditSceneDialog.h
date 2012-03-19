//
// $Id$

#ifndef EDIT_SCENE_DIALOG
#define EDIT_SCENE_DIALOG

#include "ui/Window.h"

class Button;
class Session;
class TextField;

/**
 * Allows scene owners and admins to edit the properties of the scene.
 */
class EditSceneDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    EditSceneDialog (Session* parent);

protected slots:

    /**
     * Updates the state of the update button.
     */
    void updateUpdate ();

    /**
     * Makes sure the user really wants to delete the scene.
     */
    void confirmDelete ();

    /**
     * Updates the scene.
     */
    void update ();

protected:

    /**
     * Actually deletes the scene, having confirmed that that's what the user wants.
     */
    Q_INVOKABLE void reallyDelete ();

    /** The name field. */
    TextField* _name;

    /** The scroll width field. */
    TextField* _scrollWidth;

    /** The scroll height field. */
    TextField* _scrollHeight;

    /** The update button. */
    Button* _update;
};

#endif // EDIT_SCENE_DIALOG
