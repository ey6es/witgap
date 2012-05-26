//
// $Id$

#include "RuntimeConfig.h"

void RuntimeConfig::setOpen (bool open)
{
    if (_open != open) {
        emit openChanged(_open = open);
    }
}
