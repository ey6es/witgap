//
// $Id$

#include "actor/Pawn.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneView.h"

SceneView::SceneView (Session* session) :
    Component(0)
{
    connect(session, SIGNAL(didEnterScene(Scene*)), SLOT(handleDidEnterScene(Scene*)));
    connect(session, SIGNAL(willLeaveScene(Scene*)), SLOT(handleWillLeaveScene(Scene*)));

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
        dirty();
    }
}

void SceneView::handleDidEnterScene (Scene* scene)
{
    Pawn* pawn = session()->pawn();
    if (pawn != 0) {
        // center around the pawn and adjust when it moves
        connect(pawn, SIGNAL(positionChanged(QPoint)), SLOT(maybeScroll()));
        QSize size = _bounds.size();
        setWorldBounds(QRect(pawn->position() - QPoint(size.width()/2, size.height()/2), size));
    }
    connect(scene, SIGNAL(propertiesChanged()), SLOT(maybeScroll()));
    scene->addSpatial(this);
    dirty();
}

void SceneView::handleWillLeaveScene (Scene* scene)
{
    Pawn* pawn = session()->pawn();
    if (pawn != 0) {
        disconnect(pawn);
    }
    disconnect(scene);
    scene->removeSpatial(this);
}

/**
 * Helper function for maybeScroll: returns the signed distance between the specified value and the
 * given range.
 */
static int getDelta (int value, int start, int end)
{
    return (value < start) ? (value - start) : (value > end ? (value - end) : 0);
}

void SceneView::maybeScroll ()
{
    Session* session = this->session();
    const SceneRecord& record = session->scene()->record();
    QRect scrollBounds(
        _worldBounds.left() + _worldBounds.width()/2 - record.scrollWidth/2,
        _worldBounds.top() + _worldBounds.height()/2 - record.scrollHeight/2,
        record.scrollWidth, record.scrollHeight);

    // scroll to fit the pawn position within the scroll bounds
    const QPoint& pos = session->pawn()->position();
    setWorldBounds(_worldBounds.translated(
        getDelta(pos.x(), scrollBounds.left(), scrollBounds.right()),
        getDelta(pos.y(), scrollBounds.top(), scrollBounds.bottom())));
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

    // find the intersection of the dirty bounds in world space and the world bounds
    QRect dirty = ctx->dirty().boundingRect();
    dirty.translate(-ctx->pos());
    dirty.translate(_worldBounds.topLeft());
    dirty &= _worldBounds;

    // draw all blocks that intersect
    int bx1 = dirty.left() >> Scene::Block::LgSize;
    int bx2 = dirty.right() >> Scene::Block::LgSize;
    int by1 = dirty.top() >> Scene::Block::LgSize;
    int by2 = dirty.bottom() >> Scene::Block::LgSize;
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
