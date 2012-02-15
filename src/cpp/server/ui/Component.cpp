//
// $Id$

#include "ui/Component.h"

Component::Component (QObject* parent) :
    QObject(parent)
{
}

void Component::setBounds (const QRect& bounds)
{
    _bounds = bounds;
}
