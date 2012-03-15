//
// $Id$

#ifndef ADMIN_MENU
#define ADMIN_MENU

#include "ui/Window.h"

class QKeyEvent;

class Session;

/**
 * Pops up to provide access to admin commands.
 */
class AdminMenu : public Window
{
    Q_OBJECT

public:

    /**
     * Initializes the menu.
     */
    AdminMenu (Session* parent);

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

#endif // ADMIN_MENU
