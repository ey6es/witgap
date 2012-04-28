//
// $Id$

#ifndef ADMIN_MENU
#define ADMIN_MENU

#include "ui/Menu.h"

/**
 * Pops up to provide access to admin commands.
 */
class AdminMenu : public Menu
{
    Q_OBJECT

public:

    /**
     * Initializes the menu.
     */
    Q_INVOKABLE AdminMenu (Session* parent, int deleteOnReleaseKey = -1);
};

#endif // ADMIN_MENU
