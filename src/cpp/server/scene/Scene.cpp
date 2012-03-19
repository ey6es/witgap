//
// $Id$

#include <QMetaObject>

#include "ServerApp.h"
#include "db/DatabaseThread.h"
#include "net/Session.h"
#include "scene/Scene.h"

Scene::Scene (ServerApp* app, const SceneRecord& record) :
    _record(record),
    _app(app)
{
}

bool Scene::canEdit (Session* session) const
{
    return session->admin() || session->user().id == _record.creatorId;
}

void Scene::update (const QString& name, quint16 scrollWidth, quint16 scrollHeight)
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
