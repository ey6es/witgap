//
// $Id$

#include <queue>

#include <QMetaObject>
#include <QtDebug>

#include "MainWindow.h"
#include "Protocol.h"
#include "ServerApp.h"
#include "actor/Pawn.h"
#include "db/DatabaseThread.h"
#include "net/Session.h"
#include "scene/Legend.h"
#include "scene/Scene.h"
#include "scene/SceneManager.h"
#include "scene/SceneView.h"

using namespace std;

Scene::Scene (ServerApp* app, const SceneRecord& record) :
    _app(app),
    _record(record)
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
    
    new Actor(this, '@', "Column", QPoint(0, 1), 1, 2, 0);
}

int Scene::collisionFlags (const QPoint& pos) const
{
    QPoint key(pos.x() >> Scene::Block::LgSize, pos.y() >> Scene::Block::LgSize);
    QHash<QPoint, FlagBlock>::const_iterator it = _collisionFlags.constFind(key);
    if (it == _collisionFlags.constEnd()) {
        return 0;
    }
    return (it == _collisionFlags.constEnd()) ? 0 : it->get(pos);
}

bool Scene::canEdit (Session* session) const
{
    return session->admin() || session->user().id == _record.creatorId;
}

void Scene::setProperties (const QString& name, quint16 scrollWidth, quint16 scrollHeight)
{
    // update the record
    SceneRecord record = _record;
    record.name = name;
    record.scrollWidth = scrollWidth;
    record.scrollHeight = scrollHeight;

    // update in database
    QMetaObject::invokeMethod(_app->databaseThread()->sceneRepository(), "updateScene",
        Q_ARG(const SceneRecord&, record), Q_ARG(const Callback&, Callback(
            _app->sceneManager(), "broadcastSceneUpdated(SceneRecord)",
            Q_ARG(const SceneRecord&, record))));
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
        Q_ARG(quint64, _record.id), Q_ARG(const Callback&, Callback(
            _app->sceneManager(), "broadcastSceneDeleted(quint32)",
            Q_ARG(quint32, _record.id))));
}

Pawn* Scene::addSession (Session* session, const QVariant& portal)
{
    _sessions.append(session);

    QPoint location;
    switch (portal.type()) {
        case QVariant::Point:
            location = portal.toPoint();
            break;
    }
    return new Pawn(this, session, location);
}

void Scene::removeSession (Session* session)
{
    Pawn* pawn = session->pawn();
    if (pawn != 0) {
        delete pawn;
    }
    _sessions.removeOne(session);
}

void Scene::updated (const SceneRecord& record)
{
    emit recordChanged(_record = record);
}

void Scene::deleted ()
{
    // TODO
}

void Scene::addSpatial (Actor* actor)
{
    const QPoint& pos = actor->position();
    int flags = actor->collisionFlags();
    if (flags != 0) {
        QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
        FlagBlock& block = _collisionFlags[key];
        block.setFlags(pos, flags);
    }
    
    // add to actor list
    Actor*& aref = _actors[pos];
    int character = actor->character();
    if (character == ' ') {
        // add invisible actors to the end of the list
        if (aref == 0) {
            aref = actor;

        } else {
            Actor* ptr = aref;
            while (ptr->next() != 0) {
                ptr = ptr->next();
            }
            ptr->setNext(actor);
        }
        return;
    }
    if (aref != 0) {
        actor->setNext(aref);
    }
    aref = actor;

    // set in block and dirty
    setInBlocks(pos, character, actor->labelPointer());
}

void Scene::removeSpatial (Actor* actor, int character, const QPoint* npos)
{
    // remove from actor list
    const QPoint& pos = actor->position();
    Actor*& aref = _actors[pos];
    if (aref != actor) {
        // actor is not on top, so no need to update block
        Actor* ptr = aref;
        while (ptr->next() != actor) {
            ptr = ptr->next();
        }
        ptr->setNext(actor->next());
        actor->setNext(0);
        if (actor->collisionFlags() != 0) {
            updateCollisionFlags(pos, aref);
        }
        return;
    }
    Actor* next = actor->next();
    int nchar;
    LabelPointer nlabel;
    if (next == 0) {
        _actors.remove(pos);
        nchar = _record.get(pos);
        nlabel = 0;
        
    } else {
        aref = next;
        actor->setNext(0);
        if ((nchar = next->character()) == ' ') {
            nchar = _record.get(pos);
            nlabel = 0;
            
        } else {
            nlabel = next->labelPointer();
        }
    }

    // update block and dirty
    setInBlocks(pos, nchar, nlabel, npos);
    
    // update the collision flags if appropriate
    if (actor->collisionFlags() != 0) {
        updateCollisionFlags(pos, next);
    }
}

void Scene::characterLabelChanged (Actor* actor, int ochar)
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
        setInBlocks(pos, nchar, actor->labelPointer());
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

/**
 * Contains all the information associated with a node.
 */
class Node
{
public:

    /** Whether or not the node is in the closed set. */
    unsigned int closed : 1;
    
    /** Whether or not the node is in the open set. */
    unsigned int open : 1;
    
    /** The distance from the source node. */
    unsigned int gScore : 14;
    
    /** The combined distance and heuristic estimate. */
    unsigned int fScore : 14;
    
    /** The direction from which we traveled. */
    unsigned int cameFrom : 2;
};

/**
 * Functor to compare two nodes by their scores.
 */
class NodeScoreGreater
{
public:

    /**
     * Creates a new node comparer.
     */
    NodeScoreGreater (const QHash<QPoint, Node>* nodes) : _nodes(nodes) { }

    /**
     * Returns whether or not the score of the first point is greater than that of the second.
     */
    bool operator() (const QPoint& p1, const QPoint& p2)
    {
        return _nodes->value(p1).fScore > _nodes->value(p2).fScore;
    }

protected:
    
    /** The map from point to node. */
    const QHash<QPoint, Node>* _nodes;
};

QVector<QPoint> Scene::findPath (
    const QPoint& start, const QPoint& end, int collisionMask, int maxLength) const
{
    QHash<QPoint, Node> nodes;
    NodeScoreGreater compare(&nodes);
    priority_queue<QPoint, QVector<QPoint>, NodeScoreGreater> open(compare);
    
    Node first = { false, true, 0, (start - end).manhattanLength() };
    nodes.insert(start, first);
    open.push(start);
    
    while (!open.empty()) {
        QPoint current = open.top();
        if (current == end) {
            QVector<QPoint> path;
            while (current != start) {
                path.append(current);
                
            }
            return path;
        }
        open.pop();
        Node& cnode = nodes[current];
        cnode.closed = true;
        int ngscore = cnode.gScore + 1;
        
        for (int ii = 0; ii < 4; ii++) {
            QPoint neighbor;
            Node& nnode = nodes[neighbor];
            if (nnode.closed) {
                continue;
            }
            if (!nnode.open || ngscore < nnode.gScore) {
                
            }
            
        }
    }
    
    return QVector<QPoint>();
}

void Scene::say (
    const QPoint& pos, const QString& speaker, const QString& message, ChatWindow::SpeakMode mode)
{
    if (mode == ChatWindow::ShoutMode) {
        // at least for now, shouting reaches everyone in the scene
        foreach (Session* session, _sessions) {
            session->chatWindow()->display(speaker, message, mode);
        }
        return;
    }
    QPoint key(pos.x() >> LgViewBlockSize, pos.y() >> LgViewBlockSize);
    QHash<QPoint, SceneViewList>::const_iterator it = _views.constFind(key);
    if (it != _views.constEnd()) {
        foreach (SceneView* view, *it) {
            const QRect& vbounds = view->worldBounds();
            if (vbounds.contains(pos)) {
                view->session()->chatWindow()->display(speaker, message, mode);
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

void Scene::updateCollisionFlags (const QPoint& pos, Actor* actor)
{
    int flags = 0;
    for (; actor != 0; actor = actor->next()) {
        flags |= actor->collisionFlags();
    }
    QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
    FlagBlock& block = _collisionFlags[key];
    block.set(pos, flags);
    if (block.filled() == 0) {
        _collisionFlags.remove(key);
    }
}

void Scene::setInBlocks (const QPoint& pos, int character, LabelPointer label, const QPoint* npos)
{
    QPoint key(pos.x() >> Block::LgSize, pos.y() >> Block::LgSize);
    Block& block = _blocks[key];
    int ochar = block.set(pos, character);
    if (block.filled() == 0) {
        _blocks.remove(key);
    }
    LabelBlock& lblock = _labels[key];
    LabelPointer olabel = lblock.set(pos, label);
    if (lblock.filled() == 0) {
        _labels.remove(key);
    }
    if (ochar == character && olabel == label) {
        return;
    }
    QPoint vkey(pos.x() >> LgViewBlockSize, pos.y() >> LgViewBlockSize);
    QHash<QPoint, SceneViewList>::const_iterator it = _views.constFind(vkey);
    if (it != _views.constEnd()) {
        foreach (SceneView* view, *it) {
            const QRect& vbounds = view->worldBounds();
            if (vbounds.contains(pos)) {
                if (ochar != character) {
                    static_cast<Component*>(view)->dirty(QRect(pos - vbounds.topLeft(),
                        QSize(1, 1)));
                }
                if (olabel != label) {
                    view->session()->mainWindow()->legend()->labelChanged(olabel, label, npos);
                }
            }
        }
    }
}

