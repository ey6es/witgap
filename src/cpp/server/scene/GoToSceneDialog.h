//
// $Id$

#ifndef GO_TO_SCENE_DIALOG
#define GO_TO_SCENE_DIALOG

#include "db/SceneRepository.h"
#include "ui/Window.h"

class Button;
class ScrollingList;
class Session;
class TextField;

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

protected slots:

    /**
     * Updates the selected field based on the entered name.
     */
    void updateSelection ();

    /**
     * Updates the go button based on the selection.
     */
    void updateGo ();

    /**
     * Goes to the selected scene.
     */
    void go ();

protected:

    /**
     * Populates the scene list.
     */
    Q_INVOKABLE void setScenes (const SceneDescriptorList& scenes);

    /** The scene name field. */
    TextField* _name;

    /** The list of scene names. */
    ScrollingList* _list;

    /** The "go" button. */
    Button* _go;

    /** The current list of scene descriptors. */
    SceneDescriptorList _scenes;
};

#endif // GO_TO_SCENE_DIALOG
