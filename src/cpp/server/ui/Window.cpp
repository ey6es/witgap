//
// $Id$

#include "ui/Window.h"

Window::Window (QObject* parent, int id, int layer) :
    Component(parent),
    _id(id),
    _layer(layer)
{
}

void Window::setLayer (int layer)
{
    _layer = layer;
}
