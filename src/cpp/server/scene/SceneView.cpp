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
    const QHash<QPoint, Scene::Block>& blocks = scene->blocks();

    // draw all blocks that intersect the view region
    QRect bounds = worldBounds();
    int bx1 = bounds.left() >> Scene::Block::LgSize;
    int bx2 = bounds.right() >> Scene::Block::LgSize;
    int by1 = bounds.top() >> Scene::Block::LgSize;
    int by2 = bounds.bottom() >> Scene::Block::LgSize;
    for (int by = by1; by <= by2; by++) {
        for (int bx = bx1; bx <= bx2; bx++) {
            QHash<QPoint, Scene::Block>::const_iterator it = blocks.constFind(QPoint(bx, by));
            if (it != blocks.constEnd()) {
                const Scene::Block& block = *it;
                ctx->drawContents(
                    (bx << Scene::Block::LgSize) - _location.x(),
                    (by << Scene::Block::LgSize) - _location.y(),
                    Scene::Block::Size, Scene::Block::Size, block.constData());
            }
        }
    }
}
