//
// $Id$

#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneView.h"

SceneView::SceneView (QObject* parent) :
    Component(parent)
{
}

void SceneView::draw (DrawContext* ctx) const
{
    Component::draw(ctx);

    Scene* scene = session()->scene();
    if (scene == 0) {
        return;
    }
    const QHash<QPoint, QIntVector>& contents = scene->contents();

}
