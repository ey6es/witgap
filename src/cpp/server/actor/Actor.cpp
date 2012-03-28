//
// $Id$

#include "actor/Actor.h"
#include "scene/Scene.h"

Actor::Actor (Scene* scene, int character, const QPoint& position) :
    QObject(scene),
    _scene(scene),
    _character(character),
    _position(position)
{
    _scene->addToContents(this);
}

Actor::~Actor ()
{
    _scene->removeFromContents(this);
}

void Actor::setCharacter (int character)
{
    if (_character != character) {
        _scene->removeFromContents(this);
        _character = character;
        _scene->addToContents(this);
    }
}

void Actor::setPosition (const QPoint& position)
{
    if (_position != position) {
        _scene->removeFromContents(this);
        _position = position;
        _scene->addToContents(this);
    }
}
