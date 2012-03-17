//
// $Id$

#include "ServerApp.h"
#include "scene/SceneManager.h"

SceneManager::SceneManager (ServerApp* app) :
    QObject(app),
    _app(app)
{
}
