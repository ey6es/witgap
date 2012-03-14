//
// $Id$

#ifndef COMMAND_MENU
#define COMMAND_MENU

#include "ui/Window.h"

class QKeyEvent;

class Session;

/**
 * Pops up to provide access to commands.
 */
class CommandMenu : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the menu.
     */
    CommandMenu (Session* parent);

protected:

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event.
     */
    virtual void keyReleaseEvent (QKeyEvent* e);
};

#endif // COMMAND_MENU
