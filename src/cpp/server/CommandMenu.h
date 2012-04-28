//
// $Id$

#ifndef COMMAND_MENU
#define COMMAND_MENU

#include "ui/Menu.h"

/**
 * Pops up to provide access to commands.
 */
class CommandMenu : public Menu
{
    Q_OBJECT

public:

    /**
     * Initializes the menu.
     */
    CommandMenu (Session* parent, int deleteOnReleaseKey = -1);
};

#endif // COMMAND_MENU
