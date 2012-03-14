//
// $Id$

#ifndef ADMIN_MENU
#define ADMIN_MENU

#include "ui/Window.h"

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
};

#endif // ADMIN_MENU
