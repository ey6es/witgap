//
// $Id$

#include "ServerApp.h"
#include "scene/Scene.h"

Scene::Scene (ServerApp* app, const SceneRecord& record) :
    _record(record),
    _app(app)
{
}
