//
// $Id$

#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneView.h"

SceneView::SceneView (Session* session) :
    Component(0)
{
    connect(session, SIGNAL(didEnterScene(Scene*)), SLOT(dirty()));
    connect(session, SIGNAL(didEnterScene(Scene*)), SLOT(addSpatial(Scene*)));
    connect(session, SIGNAL(willLeaveScene(Scene*)), SLOT(removeSpatial(Scene*)));

    Scene* scene = session->scene();
    if (scene != 0) {
        scene->addSpatial(this);
    }
}

SceneView::~SceneView ()
{
    Session* session = this->session();
    if (session != 0) {
        Scene* scene = session->scene();
        if (scene != 0) {
            scene->removeSpatial(this);
        }
    }
}

void SceneView::setWorldBounds (const QRect& bounds)
{
    if (_worldBounds != bounds) {
        Scene* scene = session()->scene();
        if (scene == 0) {
            _worldBounds = bounds;
        } else {
            scene->removeSpatial(this);
            _worldBounds = bounds;
            scene->addSpatial(this);
        }
    }
}

void SceneView::addSpatial (Scene* scene)
{
    scene->addSpatial(this);
}

void SceneView::removeSpatial (Scene* scene)
{
    scene->removeSpatial(this);
}

void SceneView::invalidate ()
{
    Component::invalidate();

    // resize world bounds if necessary
    setWorldBounds(QRect(_worldBounds.topLeft(), _bounds.size()));
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
    int bx1 = _worldBounds.left() >> Scene::Block::LgSize;
    int bx2 = _worldBounds.right() >> Scene::Block::LgSize;
    int by1 = _worldBounds.top() >> Scene::Block::LgSize;
    int by2 = _worldBounds.bottom() >> Scene::Block::LgSize;
    for (int by = by1; by <= by2; by++) {
        for (int bx = bx1; bx <= bx2; bx++) {
            QHash<QPoint, Scene::Block>::const_iterator it = blocks.constFind(QPoint(bx, by));
            if (it != blocks.constEnd()) {
                const Scene::Block& block = *it;
                ctx->drawContents(
                    (bx << Scene::Block::LgSize) - _worldBounds.left(),
                    (by << Scene::Block::LgSize) - _worldBounds.top(),
                    Scene::Block::Size, Scene::Block::Size, block.constData());
            }
        }
    }
}
