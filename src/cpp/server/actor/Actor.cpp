//
// $Id$

#include "actor/Actor.h"
#include "scene/Scene.h"

Actor::Actor (Scene* scene, int character, const QString& label, const QPoint& position,
        int collisionFlags, int collisionMask, int spawnMask) :
    QObject(scene),
    _scene(scene),
    _next(0),
    _position(position)
{
    _data = new ActorData();
    _data->character = character;
    if (!label.isEmpty()) {
        _data->label = QSharedPointer<CharacterLabelPair>(
            new CharacterLabelPair(character, label));
    }
    _data->collisionFlags = collisionFlags;
    _data->collisionMask = collisionMask;
    _data->spawnMask = spawnMask;
    
    _scene->addSpatial(this);
}

Actor::~Actor ()
{
    _scene->removeSpatial(this);
}

void Actor::setCharacter (int character)
{
    int ochar = this->character();
    if (ochar != character) {
        _data->character = character;
        if (!_data->label.isNull()) {
            _data->label = QSharedPointer<CharacterLabelPair>(
                new CharacterLabelPair(character, _data->label->second));
        }
        _scene->characterLabelChanged(this, ochar);
    }
}

void Actor::setLabel (const QString& label)
{
    if (this->label() != label) {
        _data->label = QSharedPointer<CharacterLabelPair>(
            label.isEmpty() ? 0 : new CharacterLabelPair(_data->character, label));
        _scene->characterLabelChanged(this, _data->character);
    }
}

void Actor::setCollisionFlags (int flags)
{
    if (collisionFlags() != flags) {
        _data->collisionFlags = flags;
    }
}

bool Actor::move (const QPoint& position)
{
    if ((_scene->collisionFlags(position) & collisionMask()) == 0) {
        setPosition(position);
        return true;
        
    } else {
        return false;
    }
}

void Actor::setPosition (const QPoint& position)
{
    if (_position != position) {
        _scene->removeSpatial(this, &position);
        QPoint opos = _position;
        _position = position;
        _scene->addSpatial(this);

        emit positionChanged(opos);
    }
}

