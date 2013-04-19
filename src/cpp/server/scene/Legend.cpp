//
// $Id$

#include <QTranslator>
#include <QtDebug>

#include "MainWindow.h"
#include "net/Session.h"
#include "scene/Legend.h"
#include "scene/SceneView.h"
#include "ui/Border.h"
#include "ui/Label.h"
#include "ui/Layout.h"

// translate through the session
#define tr(...) this->session()->translator()->translate("Legend", __VA_ARGS__)

Legend::Legend (Session* session) :
    Container(new BoxLayout(Qt::Vertical, BoxLayout::HStretch, Qt::AlignTop, 0), session)
{
    setBorder(new FrameBorder(-1, -1, -1, -1, -1, -1, '|', -1));
    setPreferredSize(QSize(20, -1));
    
    connect(session, SIGNAL(didEnterScene(Scene*)), SLOT(handleDidEnterScene(Scene*)));
    connect(session, SIGNAL(willLeaveScene(Scene*)), SLOT(handleWillLeaveScene(Scene*)));
}

/**
 * Creates a label component.
 */
Component* createLabel (const CharacterLabelPair& pair)
{
    Container* cont = new Container(new BoxLayout(
        Qt::Horizontal, BoxLayout::HStretch, Qt::AlignTop, 1));
    cont->addChild(new Label(QIntVector(1, pair.first)), BoxLayout::Fixed);
    cont->addChild(new Label(pair.second));
    return cont;
}

void Legend::labelChanged (LabelPointer olabel, LabelPointer nlabel, const QPoint* npos)
{
    if (olabel != 0) {
        QHash<LabelPointer, LabelMapping>::iterator it = _labels.find(olabel);
        if (--it->count == 0 && (npos == 0 ||
                !session()->mainWindow()->sceneView()->worldBounds().contains(*npos))) {
            removeChild(it->component);
            _labels.erase(it);
        }
    }
    if (nlabel != 0) {
        LabelMapping& mapping = _labels[nlabel];
        if (mapping.count++ == 0 && mapping.component == 0) {
            addChild(mapping.component = createLabel(*nlabel));
        }
    }
}

void Legend::handleDidEnterScene (Scene* scene)
{
    connect(session()->mainWindow()->sceneView(), SIGNAL(worldBoundsChanged(QRect)),
        SLOT(update(QRect)));
    update(QRect());
}

void Legend::handleWillLeaveScene (Scene* scene)
{
    disconnect(session()->mainWindow()->sceneView());
}

/**
 * Subtracts the second rectangle from the first, placing the resulting rectangles in the
 * provided array.
 */
void subtract (const QRect& a, const QRect& b, QRect* result)
{
    result[0].setCoords(a.left(), a.top(), a.right(), b.top() - 1);
    result[1].setCoords(a.left(), b.top(), b.left() - 1, b.bottom());
    result[2].setCoords(b.right() + 1, b.top(), a.right(), b.bottom());
    result[3].setCoords(a.left(), b.bottom() + 1, a.right(), a.bottom());
}

void Legend::update (const QRect& oldBounds)
{
    const QRect& newBounds = session()->mainWindow()->sceneView()->worldBounds();
    QRect isect = oldBounds.intersected(newBounds);
    if (isect.isEmpty()) {
        // wipe out everything, add new bounds
        removeAllChildren();
        _labels.clear();   
        add(newBounds);
        return;
    }
    // subtract old - isect, add new - isect
    QRect result[4];
    ::subtract(oldBounds, isect, result);
    for (int ii = 0; ii < 4; ii++) {
        if (!result[ii].isEmpty()) {
            subtract(result[ii]);
        }
    }
    ::subtract(newBounds, isect, result);
    for (int ii = 0; ii < 4; ii++) {
        if (!result[ii].isEmpty()) {
            add(result[ii]);
        }
    }
}

void Legend::add (const QRect& bounds)
{
    Scene* scene = session()->scene();
    const QHash<QPoint, Scene::LabelBlock>& labels = scene->labels();
    
    int bx1 = bounds.left() >> Scene::LabelBlock::LgSize;
    int bx2 = bounds.right() >> Scene::LabelBlock::LgSize;
    int by1 = bounds.top() >> Scene::LabelBlock::LgSize;
    int by2 = bounds.bottom() >> Scene::LabelBlock::LgSize;
    QRect bbounds(0, 0, Scene::LabelBlock::Size, Scene::LabelBlock::Size);
    for (int by = by1; by <= by2; by++) {
        for (int bx = bx1; bx <= bx2; bx++) {
            QHash<QPoint, Scene::LabelBlock>::const_iterator it = labels.constFind(QPoint(bx, by));
            if (it != labels.constEnd()) {
                const Scene::LabelBlock& block = *it;
                bbounds.moveTo(bx << Scene::LabelBlock::LgSize, by << Scene::LabelBlock::LgSize);
                QRect ibounds = bbounds.intersected(bounds);
                
                const LabelPointer* lptr = block.constData() +
                    (ibounds.top() - bbounds.top() << Scene::LabelBlock::LgSize) +
                        (ibounds.left() - bbounds.left());
                for (int ii = 0, nn = ibounds.height(); ii < nn; ii++) {
                    for (const LabelPointer* ptr = lptr, *end = ptr + ibounds.width();
                            ptr != end; ptr++) {
                        LabelPointer label = *ptr;
                        if (label != 0) {
                            LabelMapping& mapping = _labels[label];
                            if (mapping.count++ == 0) {
                                addChild(mapping.component = createLabel(*label));
                            }
                        }
                    }
                    lptr += Scene::LabelBlock::Size;
                }
            }
        }
    }
}

void Legend::subtract (const QRect& bounds)
{
    Scene* scene = session()->scene();
    const QHash<QPoint, Scene::LabelBlock>& labels = scene->labels();
    
    int bx1 = bounds.left() >> Scene::LabelBlock::LgSize;
    int bx2 = bounds.right() >> Scene::LabelBlock::LgSize;
    int by1 = bounds.top() >> Scene::LabelBlock::LgSize;
    int by2 = bounds.bottom() >> Scene::LabelBlock::LgSize;
    QRect bbounds(0, 0, Scene::LabelBlock::Size, Scene::LabelBlock::Size);
    for (int by = by1; by <= by2; by++) {
        for (int bx = bx1; bx <= bx2; bx++) {
            QHash<QPoint, Scene::LabelBlock>::const_iterator it = labels.constFind(QPoint(bx, by));
            if (it != labels.constEnd()) {
                const Scene::LabelBlock& block = *it;
                bbounds.moveTo(bx << Scene::LabelBlock::LgSize, by << Scene::LabelBlock::LgSize);
                QRect ibounds = bbounds.intersected(bounds);
                
                const LabelPointer* lptr = block.constData() +
                    (ibounds.top() - bbounds.top() << Scene::LabelBlock::LgSize) +
                        (ibounds.left() - bbounds.left());
                for (int ii = 0, nn = ibounds.height(); ii < nn; ii++) {
                    for (const LabelPointer* ptr = lptr, *end = ptr + ibounds.width();
                            ptr != end; ptr++) {
                        LabelPointer label = *ptr;
                        if (label != 0) {
                            QHash<LabelPointer, LabelMapping>::iterator it =
                                _labels.find(label);
                            if (--it->count == 0) {
                                removeChild(it->component);
                                _labels.erase(it);
                            }
                        }
                    }
                    lptr += Scene::LabelBlock::Size;
                }
            }
        }
    }
}
