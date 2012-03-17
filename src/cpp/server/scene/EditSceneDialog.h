//
// $Id$

#ifndef EDIT_SCENE_DIALOG
#define EDIT_SCENE_DIALOG

#include "ui/Window.h"

class Session;

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
};

#endif // EDIT_SCENE_DIALOG
