//
// $Id$

#include <QMetaObject>
#include <QtDebug>

#include "ChatWindow.h"
#include "Protocol.h"
#include "ServerApp.h"
#include "actor/Pawn.h"
#include "db/DatabaseThread.h"
#include "net/Session.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SceneView.h"

Scene::Block::Block () :
    QIntVector(Size*Size, ' '),
    _filled(0)
{
}

void Scene::Block::set (const QPoint& pos, int character)
{
    int& value = (*this)[(pos.y() & Mask) << LgSize | pos.x() & Mask];
    _filled += (value == ' ') - (character == ' ');
    value = character;
}

Scene::Scene (ServerApp* app, const SceneRecord& record) :
    _record(record),
    _app(app)
{
}

Scene::~Scene ()
{
}

void Scene::init ()
{
    // initialize the contents from the record
    for (QHash<QPoint, SceneRecord::Block>::const_iterator it = _record.blocks.constBegin(),
            end = _record.blocks.constEnd(); it != end; it++) {
        const QPoint& key = it.key();
        const SceneRecord::Block& sblock = it.value();

        // find the extents of the source block (inclusive)
        int sx1 = key.x() << SceneRecord::Block::LgSize;
        int sx2 = sx1 + SceneRecord::Block::Size - 1;
        int sy1 = key.y() << SceneRecord::Block::LgSize;
        int sy2 = sy1 + SceneRecord::Block::Size - 1;

        // map them to destination blocks
        int bx1 = sx1 >> Scene::Block::LgSize;
        int bx2 = sx2 >> Scene::Block::LgSize;
        int by1 = sy1 >> Scene::Block::LgSize;
        int by2 = sy2 >> Scene::Block::LgSize;

        // iterate over intersected destination blocks
        for (int by = by1; by <= by2; by++) {
            for (int bx = bx1; bx <= bx2; bx++) {
                Block& dblock = _blocks[QPoint(bx, by)];

                // find the extents of the destination block
                int dx1 = bx << Scene::Block::LgSize;
                int dx2 = dx1 + Scene::Block::Size - 1;
                int dy1 = by << Scene::Block::LgSize;
                int dy2 = dy1 + Scene::Block::Size - 1;

                // find the intersection extents
                int x1 = qMax(sx1, dx1);
                int x2 = qMin(sx2, dx2);
                int y1 = qMax(sy1, dy1);
                int y2 = qMin(sy2, dy2);

                // copy/process the area of intersection
                const int* lsptr = sblock.constData() +
                    (y1 - sy1 << SceneRecord::Block::LgSize) + (x1 - sx1);
                int* ldptr = dblock.data() + (y1 - dy1 << Scene::Block::LgSize) + (x1 - dx1);
                int filled = dblock.filled();
                for (int yy = y1; yy <= y2; yy++) {
                    const int *sptr = lsptr;
                    int* dptr = ldptr;
                    for (int xx = x1; xx <= x2; xx++) {
                        if ((*dptr++ = *sptr++) != ' ') {
                            filled++;
                        }
                    }
                    lsptr += SceneRecord::Block::Size;
                    ldptr += Scene::Block::Size;
                }
                dblock.setFilled(filled);
            }
        }
    }
}

bool Scene::canEdit (Session* session) const
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

    // notify listeners
    emit propertiesChanged();
}

void Scene::set (const QPoint& pos, int character)
{
    // set in the record
    _record.set(pos, character);

    // notify the top actor at the position, if any; otherwise, set in the contents
    Actor* actor = _actors.value(pos);
    if (actor != 0) {
        actor->sceneChangedUnderneath(character);
    } else {
        setInBlocks(pos, character);
    }
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
    return new Pawn(this, session, QPoint(0, 0));
}

void Scene::removeSession (Session* session)
{
    Pawn* pawn = session->pawn();
    if (pawn != 0) {
        delete pawn;
    }
    session->setParent(0);
}

void Scene::addSpatial (Actor* actor)
{
    // ignore invisible actors
    int character = actor->character();
    if (character == ' ') {
        return;
    }

    // add to actor list
    const QPoint& pos = actor->position();
    Actor*& aref = _actors[pos];
    if (aref != 0) {
        actor->setNext(aref);
    }
    aref = actor;

    // set in block and dirty
    setInBlocks(pos, character);
}

void Scene::removeSpatial (Actor* actor, int character)
{
    // ignore invisible actors
    if (character == ' ') {
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
    int nchar;
    if (next == 0) {
        _actors.remove(pos);
        nchar = _record.get(pos);
    } else {
        aref = next;
        actor->setNext(0);
        nchar = next->character();
    }

    // update block and dirty
    setInBlocks(pos, nchar);
}

void Scene::characterChanged (Actor* actor, int ochar)
{
    // handle transitions between visible and invisible
    int nchar = actor->character();
    if (nchar == ' ') {
        if (ochar != ' ') {
            removeSpatial(actor, ochar);
        }
        return;
    }
    if (ochar == ' ') {
        addSpatial(actor);
        return;
    }
    const QPoint& pos = actor->position();
    Actor* pactor = _actors.value(pos);
    if (pactor == actor) {
        setInBlocks(pos, nchar);
    }
}

void Scene::addSpatial (SceneView* view)
{
    // add to lists for all intersecting blocks
    const QRect& bounds = view->worldBounds();
    int x1 = bounds.left() >> LgViewBlockSize;
    int x2 = bounds.right() >> LgViewBlockSize;
    int y1 = bounds.top() >> LgViewBlockSize;
    int y2 = bounds.bottom() >> LgViewBlockSize;
    for (int yy = y1; yy <= y2; yy++) {
        for (int xx = x1; xx <= x2; xx++) {
            _views[QPoint(xx, yy)].append(view);
        }
    }
}

void Scene::removeSpatial (SceneView* view)
{
    // remove from lists in all intersecting blocks
    const QRect& bounds = view->worldBounds();
    int x1 = bounds.left() >> LgViewBlockSize;
    int x2 = bounds.right() >> LgViewBlockSize;
    int y1 = bounds.top() >> LgViewBlockSize;
    int y2 = bounds.bottom() >> LgViewBlockSize;
    for (int yy = y1; yy <= y2; yy++) {
        for (int xx = x1; xx <= x2; xx++) {
            QPoint key(xx, yy);
            SceneViewList& list = _views[key];
            list.removeOne(view);
            if (list.isEmpty()) {
                _views.remove(key);
            }
        }
    }
}

void Scene::say (const QPoint& pos, const QString& speaker, const QString& message)
{
    QPoint key(pos.x() >> LgViewBlockSize, pos.y() >> LgViewBlockSize);
    QHash<QPoint, SceneViewList>::const_iterator it = _views.constFind(key);
    if (it != _views.constEnd()) {
        foreach (SceneView* view, *it) {
            const QRect& vbounds = view->worldBounds();
            if (vbounds.contains(pos)) {
                view->session()->chatWindow()->displayMessage(speaker, message);
            }
        }
    }
}

void Scene::flush ()
{
    if (_record.dirty()) {
        QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "updateSceneBlocks",
            Q_ARG(const SceneRecord&, _record));
        _record.clean();
    }
}

void Scene::setInBlocks (const QPoint& pos, int character)
{
    QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
    Block& block = _blocks[key];
    block.set(pos, character);
    if (block.filled() == 0) {
        _blocks.remove(key);
    }
    dirty(pos);
}

void Scene::dirty (const QPoint& pos)
{
    QPoint vkey(pos.x() >> LgViewBlockSize, pos.y() >> LgViewBlockSize);
    QHash<QPoint, SceneViewList>::const_iterator it = _views.constFind(vkey);
    if (it != _views.constEnd()) {
        foreach (SceneView* view, *it) {
            const QRect& vbounds = view->worldBounds();
            if (vbounds.contains(pos)) {
                view->dirty(QRect(pos - vbounds.topLeft(), QSize(1, 1)));
            }
        }
    }
}
