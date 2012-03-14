//
// $Id$

#include "scene/SceneView.h"

SceneView::SceneView (QObject* parent) :
    Component(parent)
{
}

void SceneView::draw (DrawContext* ctx) const
{
    Component::draw(ctx);


}
