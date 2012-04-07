//
// $Id$

#include "actor/Actor.h"
#include "scene/Scene.h"

Actor::Actor (Scene* scene, int character, const QPoint& position) :
    QObject(scene),
    _scene(scene),
    _next(0),
    _character(character),
    _position(position)
{
    _scene->addSpatial(this);
}

Actor::~Actor ()
{
    _scene->removeSpatial(this);
}

void Actor::setCharacter (int character)
{
    if (_character != character) {
        _scene->removeSpatial(this);
        _character = character;
        _scene->addSpatial(this);
    }
}

void Actor::setPosition (const QPoint& position)
{
    if (_position != position) {
        _scene->removeSpatial(this);
        _position = position;
        _scene->addSpatial(this);
    }
}
