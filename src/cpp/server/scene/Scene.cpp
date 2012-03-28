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
    _contents = _record.data;
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
    actor->position();
}

void Scene::removeFromContents (Actor* actor)
{
}
