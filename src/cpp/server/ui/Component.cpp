//
// $Id$

#include <QCoreApplication>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimerEvent>

#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Component.h"
#include "ui/Layout.h"
#include "ui/Window.h"

Component::Component (QObject* parent) :
    CallableObject(parent),
    _border(0),
    _explicitPreferredSize(-1, -1),
    _background(0),
    _valid(false),
    _focused(false),
    _enabled(true),
    _visible(true)
{
    // connect slots
    connect(this, SIGNAL(boundsChanged()), SLOT(invalidate()));
}

Component::~Component ()
{
    if (_border != 0) {
        delete _border;
    }
}

Window* Component::window ()
{
    for (QObject* obj = this; obj != 0; obj = obj->parent()) {
        Window* window = qobject_cast<Window*>(obj);
        if (window != 0) {
            return window;
        }
    }
    return 0;
}

Session* Component::session () const
{
    for (QObject* obj = parent(); obj != 0; obj = obj->parent()) {
        Session* session = qobject_cast<Session*>(obj);
        if (session != 0) {
            return session;
        }
    }
    return 0;
}

void Component::setBounds (const QRect& bounds)
{
    if (_bounds != bounds) {
        // add the bounds as dirty before and after the change
        dirty();
        _bounds = bounds;
        dirty();
        emit boundsChanged();
    }
}

QRect Component::localBounds () const
{
    return QRect(0, 0, _bounds.width(), _bounds.height());
}

QRect Component::innerRect () const
{
    return QRect(_margins.left(), _margins.top(),
        _bounds.width() - (_margins.left() + _margins.right()),
        _bounds.height() - (_margins.top() + _margins.bottom()));
}

QPoint Component::absolutePos () const
{
    Container* container = qobject_cast<Container*>(parent());
    return (container == 0) ? _bounds.topLeft() : container->absolutePos() + _bounds.topLeft();
}

void Component::setBorder (Border* border)
{
    if (_border != border) {
        if (_border != 0) {
            delete _border;
        }
        _border = border;
        updateMargins();
        invalidate();
        dirty();
    }
}

void Component::setBackground (int background)
{
    _background = background;
    dirty();
}

void Component::setPreferredSize (const QSize& size)
{
    if (_explicitPreferredSize != size) {
        _explicitPreferredSize = size;
        invalidateParent();
    }
}

QSize Component::preferredSize (int whint, int hhint) const
{
    // replace hints with explicit size, if provided
    int ewidth = _explicitPreferredSize.width();
    int eheight = _explicitPreferredSize.height();
    if (ewidth != -1) {
        if (eheight != -1) {
            return _explicitPreferredSize;
        }
        whint = ewidth;

    } else if (eheight != -1) {
        hhint = eheight;
    }

    // if not, we must compute; first get the border margins
    int mwidth = _margins.left() + _margins.right();
    int mheight = _margins.top() + _margins.bottom();

    // subtract them from the hints, if provided
    if (whint != -1) {
        whint -= mwidth;
    }
    if (hhint != -1) {
        hhint -= mheight;
    }

    // compute the preferred size
    QSize computed = computePreferredSize(whint, hhint);

    // add back the margins
    return QSize(
        (ewidth == -1) ? (computed.width() + mwidth) : ewidth,
        (eheight == -1) ? (computed.height() + mheight) : eheight);
}

void Component::setConstraint (const QVariant& constraint)
{
    if (_constraint != constraint) {
        _constraint = constraint;
        invalidateParent();
    }
}

void Component::setEnabled (bool enabled)
{
    if (_enabled != enabled) {
        _enabled = enabled;
        dirty();
    }
}

void Component::setVisible (bool visible)
{
    if (_visible != visible) {
        _visible = visible;
        invalidate();
    }
}

void Component::maybeValidate ()
{
    if (_visible && !_valid) {
        validate();
        _valid = true;
    }
}

void Component::maybeDraw (DrawContext* ctx)
{
    if (_visible && ctx->isDirty(_bounds)) {
        QPoint tl = _bounds.topLeft();
        ctx->translate(tl);

        draw(ctx);

        ctx->untranslate(tl);
    }
}

Component* Component::componentAt (QPoint pos, QPoint* relative)
{
    *relative = pos;
    return this;
}

bool Component::transferFocus (Component* from, Direction dir)
{
    if (from == this) {
        Container* container = qobject_cast<Container*>(parent());
        return (container != 0) ? container->transferFocus(this, dir) : transferFocus(0, dir);
    }
    if (acceptsFocus()) {
        window()->setFocus(this);
        return true;
    }
    return false;
}

bool Component::event (QEvent* e)
{
    QObject* parent = this->parent();
    switch (e->type()) {
        case QEvent::FocusIn:
            _focused = true;
            focusInEvent((QFocusEvent*)e);
            return true;

        case QEvent::FocusOut:
            _focused = false;
            focusOutEvent((QFocusEvent*)e);
            return true;

        case QEvent::MouseButtonPress: {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            mouseButtonPressEvent(me);
            if (me->isAccepted()) {
                return true;
            }
            QMouseEvent nme(me->type(), me->pos() + _bounds.topLeft(),
                me->globalPos(), me->button(), me->buttons(), me->modifiers());
            return parent != 0 && QCoreApplication::sendEvent(parent, &nme);
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent* me = static_cast<QMouseEvent*>(e);
            mouseButtonReleaseEvent(me);
            if (me->isAccepted()) {
                return true;
            }
            QMouseEvent nme(me->type(), me->pos() + _bounds.topLeft(),
                me->globalPos(), me->button(), me->buttons(), me->modifiers());
            return parent != 0 && QCoreApplication::sendEvent(parent, &nme);
        }
        case QEvent::KeyPress:
            keyPressEvent((QKeyEvent*)e);
            return e->isAccepted() || (parent != 0 && QCoreApplication::sendEvent(parent, e));

        case QEvent::KeyRelease:
            keyReleaseEvent((QKeyEvent*)e);
            return e->isAccepted() || (parent != 0 && QCoreApplication::sendEvent(parent, e));

        case QEvent::Timer:
            timerEvent((QTimerEvent*)e);
            return e->isAccepted();

        default:
            return QObject::event(e);
    }
}

void Component::requestFocus ()
{
    session()->requestFocus(this);
}

void Component::invalidate ()
{
    if (_valid) {
        _valid = false;

        // invalidate all the way up the line
        invalidateParent();
    }
}

void Component::dirty (const QRect& region)
{
    // translate into parent coordinates
    Component* pcomp = qobject_cast<Component*>(parent());
    if (pcomp != 0) {
        pcomp->dirty(region.translated(_bounds.topLeft()));
    }
}

void Component::scrollDirty (const QPoint& delta)
{
    scrollDirty(QRect(0, 0, _bounds.width(), _bounds.height()), delta);
}

void Component::scrollDirty (const QRect& region, const QPoint& delta)
{
    // translate into parent coordinates
    Component* pcomp = qobject_cast<Component*>(parent());
    if (pcomp != 0) {
        pcomp->scrollDirty(region.translated(_bounds.topLeft()), delta);
    }
}

void Component::updateMargins ()
{
    _margins = (_border == 0) ? QMargins() : _border->margins();
}

QSize Component::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, 0), qMax(hhint, 0));
}

void Component::validate ()
{
    // nothing by default
}

void Component::draw (DrawContext* ctx)
{
    // draw the background
    ctx->fillRect(0, 0, _bounds.width(), _bounds.height(), _background);

    // draw the border
    int width = _bounds.width(), height = _bounds.height();
    if (_border != 0) {
        _border->draw(ctx, 0, 0, _bounds.width(), _bounds.height());
    }
}

void Component::focusInEvent (QFocusEvent* e)
{
}

void Component::focusOutEvent (QFocusEvent* e)
{
}

void Component::mouseButtonPressEvent (QMouseEvent* e)
{
    if (!_focused && acceptsFocus()) {
        requestFocus();

    } else {
        e->ignore();
    }
}

void Component::mouseButtonReleaseEvent (QMouseEvent* e)
{
    e->ignore(); // pass up to parent
}

/**
 * Helper function for keyPressEvent: returns the focus traversal direction.
 */
Component::Direction getDirection (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    int key = e->key();
    if (key == Qt::Key_Tab) {
        if (modifiers == Qt::ShiftModifier) {
            return Component::Backward;
        } else if (modifiers == Qt::NoModifier) {
            return Component::Forward;
        } else {
            return Component::NoDirection;
        }
    }
    if (modifiers != Qt::NoModifier) {
        return Component::NoDirection; // no modifiers allowed
    }
    switch (key) {
        case Qt::Key_Left:
            return Component::Left;
        case Qt::Key_Right:
            return Component::Right;
        case Qt::Key_Up:
            return Component::Up;
        case Qt::Key_Down:
            return Component::Down;
        default:
            return Component::NoDirection;
    }
}

void Component::keyPressEvent (QKeyEvent* e)
{
    // handle focus traversal
    if (_focused) {
        Direction dir = getDirection(e);
        if (dir != NoDirection) {
            transferFocus(this, dir);
            return;
        }
    }
    e->ignore(); // pass up to parent
}

void Component::keyReleaseEvent (QKeyEvent* e)
{
    e->ignore(); // pass up to parent
}

void Component::timerEvent (QTimerEvent* e)
{
}

void Component::invalidateParent () const
{
    Component* pcomp = qobject_cast<Component*>(parent());
    if (pcomp != 0) {
        pcomp->invalidate();
    }
}

Spacer::Spacer (int width, int height, int background, QObject* parent) :
    Component(parent)
{
    _explicitPreferredSize = QSize(width, height);
    _background = background;
}

Container::Container (Layout* layout, QObject* parent) :
    Component(parent),
    _layout(layout)
{
}

Container::~Container ()
{
    if (_layout != 0) {
        delete _layout;
    }
}

void Container::setLayout (Layout* layout)
{
    if (_layout != layout) {
        if (_layout != 0) {
            delete _layout;
        }
        _layout = layout;
        invalidate();
    }
}

void Container::addChild (Component* child, const QVariant& constraint)
{
    _children.append(child);
    child->setParent(this);
    child->setConstraint(constraint);
    dirty(child->bounds());
    invalidate();
}

/**
 * Helper function for removal: clears the parent or deletes the object, depending on
 * the value of destroy.
 */
void destroyOrUnparent (Component* child, bool destroy)
{
    if (destroy) {
        delete child;
    } else {
        child->setParent(0);
    }
}

void Container::removeChild (Component* child, bool destroy)
{
    if (_children.removeOne(child)) {
        dirty(child->bounds());
        destroyOrUnparent(child, destroy);
        invalidate();
    }
}

void Container::removeChildAt (int idx, bool destroy)
{
    Component* child = _children.takeAt(idx);
    dirty(child->bounds());
    destroyOrUnparent(child, destroy);
    invalidate();
}

void Container::removeAllChildren (bool destroy)
{
    if (_children.isEmpty()) {
        return;
    }
    foreach (Component* child, _children) {
        dirty(child->bounds());
        destroyOrUnparent(child, destroy);
    }
    _children.clear();
    invalidate();
}

int Container::visibleChildCount () const
{
    int total = 0;
    foreach (Component* child, _children) {
        if (child->visible()) {
            total++;
        }
    }
    return total;
}

Component* Container::componentAt (QPoint pos, QPoint* relative)
{
    foreach (Component* child, _children) {
        if (child->bounds().contains(pos)) {
            Component* comp = child->componentAt(pos - child->bounds().topLeft(), relative);
            if (comp != 0) {
                return comp;
            }
        }
    }
    return Component::componentAt(pos, relative);
}

bool Container::transferFocus (Component* from, Direction dir)
{
    if (!(_enabled && _visible)) {
        return false;
    }
    if (dir == Forward) {
        int ii = (from == 0) ? 0 : _children.indexOf(from) + 1;
        for (int nn = _children.length(); ii < nn; ii++) {
            if (_children[ii]->transferFocus(0, dir)) {
                return true;
            }
        }
    } else if (dir == Backward) {
        int ii = (from == 0) ? _children.length() - 1 : _children.indexOf(from) - 1;
        for (; ii >= 0; ii--) {
            if (_children[ii]->transferFocus(0, dir)) {
                return true;
            }
        }
    } else {
        // for directions other than forward/back, we delegate to the layout (if any)
        if (_layout != 0) {
            return _layout->transferFocus(this, from, dir);
        }
        return transferFocus(from, (dir == Right || dir == Down) ? Forward : Backward);
    }
    return from != 0 && Component::transferFocus(this, dir);
}

QSize Container::computePreferredSize (int whint, int hhint) const
{
    // if we have a layout, that computes the size
    if (_layout != 0) {
        return _layout->computePreferredSize(this, whint, hhint);
    }

    // otherwise, use the bounds of the children
    int mwidth = qMax(whint, 0);
    int mheight = qMax(hhint, 0);
    foreach (Component* child, _children) {
        QRect bounds = child->bounds();
        mwidth = qMax(bounds.x() + bounds.width(), mwidth);
        mheight = qMax(bounds.y() + bounds.height(), mheight);
    }
    return QSize(mwidth, mheight);
}

void Container::validate ()
{
    // apply the layout, if any
    if (_layout != 0) {
        _layout->apply(this);
    }

    // validate all children
    foreach (Component* child, _children) {
        child->maybeValidate();
    }
}

void Container::draw (DrawContext* ctx)
{
    Component::draw(ctx);

    // draw the children
    foreach (Component* child, _children) {
        child->maybeDraw(ctx);
    }
}

void DrawContext::drawChar (int x, int y, int ch)
{
    // zero implies transparent
    if (ch == 0) {
        return;
    }

    // apply offset
    x += _pos.x();
    y += _pos.y();

    // check against each rect
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        if (rect.contains(x, y)) {
            _buffers[ii][(y - rect.y())*rect.width() + (x - rect.x())] = ch;
            return;
        }
    }
}

void DrawContext::fillRect (int x, int y, int width, int height, int ch, bool force)
{
    // zero implies transparent
    if (ch == 0 && !force) {
        return;
    }

    // apply offset
    x += _pos.x();
    y += _pos.y();

    // check against each rect
    QRect fill(x, y, width, height);
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        QRect isect = rect.intersected(fill);
        if (isect.isEmpty()) {
            continue;
        }
        int stride = rect.width();
        int* ptr = _buffers[ii].data() + (isect.y() - rect.y())*stride + (isect.x() - rect.x());
        for (int yy = isect.height(); yy > 0; yy--) {
            qFill(ptr, ptr + isect.width(), ch);
            ptr += stride;
        }
    }
}

void DrawContext::drawContents (
    int x, int y, int width, int height, const int* contents, bool opaque)
{
    // apply offset
    x += _pos.x();
    y += _pos.y();

    // check against each rect
    QRect draw(x, y, width, height);
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        QRect isect = rect.intersected(draw);
        if (isect.isEmpty()) {
            continue;
        }
        int stride = rect.width();
        int* ldptr = _buffers[ii].data() + (isect.y() - rect.y())*stride + (isect.x() - rect.x());
        const int* lsptr = contents + (isect.y() - y)*width + (isect.x() - x);

        // if we're opaque, we can just copy line-by-line; otherwise, we must check for zero values
        if (opaque) {
            for (int yy = isect.height(); yy > 0; yy--) {
                qCopy(lsptr, lsptr + isect.width(), ldptr);
                lsptr += width;
                ldptr += stride;
            }
        } else {
            int ch;
            for (int yy = isect.height(); yy > 0; yy--) {
                const int* sptr = lsptr;
                for (int* dptr = ldptr, *end = ldptr + isect.width(); dptr < end; dptr++) {
                    if ((ch = *sptr++) != 0) {
                        *dptr = ch;
                    }
                }
                lsptr += width;
                ldptr += stride;
            }
        }
    }
}

void DrawContext::drawString (int x, int y, const QString& string, int style)
{
    drawString(x, y, string.constData(), string.length(), style);
}

void DrawContext::drawString (int x, int y, const QChar* string, int length, int style)
{
    // apply offset
    x += _pos.x();
    y += _pos.y();

    // check against each rect
    QRect draw(x, y, length, 1);
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        QRect isect = rect.intersected(draw);
        if (isect.isEmpty()) {
            continue;
        }
        int stride = rect.width();
        int* dptr = _buffers[ii].data() + (isect.y() - rect.y())*stride + (isect.x() - rect.x());
        for (const QChar* sptr = string + (isect.x() - x), *end = sptr + isect.width();
                sptr < end; sptr++) {
            *dptr++ = (*sptr).unicode() | style;
        }
    }
}

void DrawContext::moveContents (int x, int y, int width, int height, int dx, int dy)
{
    moveContents(QRect(x + _pos.x(), y + _pos.y(), width, height),
        QPoint(dx + _pos.x(), dy + _pos.y()));
}

void DrawContext::prepareForDrawing ()
{
    // get the list of dirty rects
    _rects = _dirty.rects();

    // create the corresponding buffers
    int nrects = _rects.size();
    _buffers.resize(nrects);
    for (int ii = 0; ii < nrects; ii++) {
        const QRect& rect = _rects.at(ii);
        _buffers[ii].resize(rect.width() * rect.height());
    }
}
