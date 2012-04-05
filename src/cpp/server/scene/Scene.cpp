//
// $Id$

#include <QMetaObject>

#include "Protocol.h"
#include "ServerApp.h"
#include "actor/Pawn.h"
#include "db/DatabaseThread.h"
#include "net/Session.h"
#include "scene/Scene.h"

Scene::Scene (ServerApp* app, const SceneRecord& record) :
    _record(record),
    _app(app)
{
    // initialize the contents from the record
//    _contents = _record.data;
}

bool Scene::canSetProperties (Session* session) const
{
    return session->admin() || session->user().id == _record.creatorId;
}

void Scene::setProperties (const QString& name, quint16 scrollWidth, quint16 scrollHeight)
{
    // update the record
    _record.name = name;
    _record.scrollWidth = scrollWidth;
    _record.scrollHeight = scrollHeight;

    // update in database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "updateScene",
        Q_ARG(const SceneRecord&, _record));
}

void Scene::remove ()
{
    // delete from the database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "deleteScene",
        Q_ARG(quint64, _record.id));
}

Pawn* Scene::addSession (Session* session)
{
    session->setParent(this);
    return new Pawn(this, session->record().avatar, QPoint(0, 0));
}

void Scene::removeSession (Session* session)
{
    Pawn* pawn = session->pawn();
    if (pawn != 0) {
        delete pawn;
    }
    session->setParent(0);
}

void Scene::addToContents (Actor* actor)
{
    // ignore invisible actors
    int character = actor->character();
    if (character == ' ') {
        return;
    }

    // add to block
    const QPoint& pos = actor->position();
    QPoint key(pos.x() >> SceneBlock::LgSize, pos.y() >> SceneBlock::LgSize);
    _blocks[key].set(pos, character);

    // add to actor list
    Actor*& aref = _actors[pos];
    if (aref != 0) {
        actor->setNext(aref);
    }
    aref = actor;
}

void Scene::removeFromContents (Actor* actor)
{
    // ignore invisible actors
    if (actor->character() == ' ') {
        return;
    }

    // remove from actor list
    const QPoint& pos = actor->position();
    Actor*& aref = _actors[pos];
    if (aref != actor) {
        // actor is not on top, so no need to update block
        Actor* pactor = aref;
        while (pactor->next() != actor) {
            pactor = pactor->next();
        }
        pactor->setNext(actor->next());
        actor->setNext(0);
        return;
    }
    Actor* next = actor->next();
    int character;
    if (next == 0) {
        _actors.remove(pos);
        character = ' ';
    } else {
        aref = next;
        actor->setNext(0);
        character = next->character();
    }

    // update block
    QPoint key(pos.x() >> SceneBlock::LgSize, pos.y() >> SceneBlock::LgSize);
    SceneBlock& block = _blocks[key];
    block.set(pos, character);
    if (block.filled() == 0) {
        _blocks.remove(key);
    }
}

SceneBlock::SceneBlock () :
    QIntVector(Size*Size, ' '),
    _filled(0)
{
}

void SceneBlock::set (const QPoint& pos, int character)
{
    int& value = (*this)[(pos.y() & Mask) << LgSize | pos.x() & Mask];
    bool oempty = (value == ' ');
    value = character;
    if (character == ' ') {
        if (!oempty) {
            _filled--;
        }
    } else if (oempty) {
        _filled++;
    }
}
