//
// $Id$

#ifndef SCENE_PROPERTIES_DIALOG
#define SCENE_PROPERTIES_DIALOG

#include "ui/Window.h"

class Button;
class Session;
class TextField;

/**
 * Allows scene owners and admins to edit the properties of the scene.
 */
class ScenePropertiesDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    ScenePropertiesDialog (Session* parent);

protected slots:

    /**
     * Updates the state of the apply/OK buttons.
     */
    void updateApply ();

    /**
     * Makes sure the user really wants to delete the scene.
     */
    void confirmDelete ();

    /**
     * Applies the changes.
     */
    void apply ();

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

    /** The apply/OK buttons. */
    Button* _apply, *_ok;
};

#endif // SCENE_PROPERTIES_DIALOG
