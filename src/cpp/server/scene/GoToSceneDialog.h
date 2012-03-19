//
// $Id$

#ifndef GO_TO_SCENE_DIALOG
#define GO_TO_SCENE_DIALOG

#include "ui/Window.h"

class Session;

/**
 * Allows users to search for scenes by name and go to them.
 */
class GoToSceneDialog : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the dialog.
     */
    GoToSceneDialog (Session* parent);
};

#endif // GO_TO_SCENE_DIALOG
