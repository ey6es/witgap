//
// $Id$

#ifndef MAIN_WINDOW
#define MAIN_WINDOW

#include "ui/Window.h"

class SceneView;

/**
 * The main window, which contains the scene view.
 */
class MainWindow : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the window.
     */
    MainWindow (Session* parent);

public slots:

    /**
     * Updates the window title (which contains the session name).
     */
    void updateTitle ();

protected:

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event.
     */
    virtual void keyReleaseEvent (QKeyEvent* e);

    /** The scene view. */
    SceneView* _sceneView;
};

#endif // MAIN_WINDOW
