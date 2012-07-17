//
// $Id$

#include "actor/Actor.h"
#include "scene/Scene.h"

Actor::Actor (Scene* scene, int character, const QPoint& position, const QString& label) :
    QObject(scene),
    _scene(scene),
    _next(0),
    _character(character),
    _position(position),
    _label(label)
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
        int ochar = _character;
        _character = character;
        _scene->characterChanged(this, ochar);
    }
}

void Actor::setPosition (const QPoint& position)
{
    if (_position != position) {
        _scene->removeSpatial(this);
        QPoint opos = _position;
        _position = position;
        _scene->addSpatial(this);

        emit positionChanged(opos);
    }
}

void Actor::setLabel (const QString& label)
{
    if (_label != label) {
        _label = label;
    }
}
