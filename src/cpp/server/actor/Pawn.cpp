//
// $Id$

#include <QKeyEvent>

#include "Protocol.h"
#include "actor/Pawn.h"
#include "scene/Scene.h"

Pawn::Pawn (Scene* scene, QChar character, const QPoint& position) :
    Actor(scene, character.unicode() | REVERSE_FLAG, position)
{
}

void Pawn::keyPressEvent (QKeyEvent* e)
{
    QString text = e->text();
    if (!text.isEmpty()) {
        QChar character = text.at(0);
        if (character.isPrint()) {
            _scene->set(_position, character.unicode());
            setPosition(QPoint(_position.x() + 1, _position.y()));
            return;
        }
    }
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier) {
        e->ignore();
        return;
    }
    switch (e->key()) {
        case Qt::Key_Left:
            setPosition(QPoint(_position.x() - 1, _position.y()));
            break;

        case Qt::Key_Right:
            setPosition(QPoint(_position.x() + 1, _position.y()));
            break;

        case Qt::Key_Up:
            setPosition(QPoint(_position.x(), _position.y() - 1));
            break;

        case Qt::Key_Down:
            setPosition(QPoint(_position.x(), _position.y() + 1));
            break;

        default:
            e->ignore();
            break;
    }
}

void Pawn::keyReleaseEvent (QKeyEvent* e)
{
    e->ignore();
}
