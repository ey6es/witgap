//
// $Id$

#include <QKeyEvent>

#include "Protocol.h"
#include "actor/Pawn.h"
#include "net/Session.h"
#include "scene/Scene.h"

Pawn::Pawn (Scene* scene, Session* session, const QPoint& position) :
    Actor(scene, session->record().avatar.unicode(), position, session->record().name),
    _session(session),
    _cursor(false)
{
}

void Pawn::setCursor (bool cursor)
{
    if (_cursor != cursor) {
        if (_cursor = cursor) {
            connect(this, SIGNAL(positionChanged(QPoint)), SLOT(updateCharacter()));
        } else {
            disconnect(this);
        }
        updateCharacter();
    }
}

void Pawn::keyPressEvent (QKeyEvent* e)
{
    QString text = e->text();
    if (_cursor && !text.isEmpty()) {
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
        case Qt::Key_Backspace:
            if (_cursor) {
                setPosition(QPoint(_position.x() - 1, _position.y()));
                _scene->set(_position, ' ');
            }
            break;

        case Qt::Key_Delete:
            if (_cursor) {
                _scene->set(_position, ' ');
            }
            break;

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

void Pawn::sceneChangedUnderneath (int character)
{
    if (_cursor) {
        setCharacter(character | REVERSE_FLAG);
    }
}

void Pawn::updateCharacter ()
{
    setCharacter(_cursor ? (_scene->record().get(_position) | REVERSE_FLAG) :
        _session->record().avatar.unicode());
}
