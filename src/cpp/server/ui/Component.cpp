//
// $Id$

#include "net/Session.h"
#include "ui/Border.h"
#include "ui/Component.h"
#include "ui/Layout.h"

Component::Component (QObject* parent) :
    QObject(parent),
    _border(0),
    _explicitPreferredSize(-1, -1),
    _background(0),
    _valid(false),
    _focused(false)
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

QRect Component::innerRect () const
{
    return QRect(_margins.left(), _margins.top(),
        _bounds.width() - (_margins.left() + _margins.right()),
        _bounds.height() - (_margins.top() + _margins.bottom()));
}

void Component::setBorder (Border* border)
{
    if (_border != border) {
        if (_border != 0) {
            delete _border;
        }
        _border = border;
        _margins = (border == 0) ? QMargins() : border->margins();
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

void Component::maybeValidate ()
{
    if (!_valid) {
        validate();
        _valid = true;
    }
}

void Component::maybeDraw (DrawContext* ctx) const
{
    if (ctx->isDirty(_bounds)) {
        QPoint tl = _bounds.topLeft();
        ctx->translate(tl);

        draw(ctx);

        ctx->untranslate(tl);
    }
}

void Component::requestFocus ()
{
    session()->setFocus(this);
}

bool Component::event (QEvent* e)
{
    switch (e->type()) {
        case QEvent::FocusIn:
            _focused = true;
            focusInEvent((QFocusEvent*)e);
            return true;

        case QEvent::FocusOut:
            _focused = false;
            focusOutEvent((QFocusEvent*)e);
            return true;

        case QEvent::KeyPress:
            keyPressEvent((QKeyEvent*)e);
            return e->isAccepted();

        case QEvent::KeyRelease:
            keyReleaseEvent((QKeyEvent*)e);
            return e->isAccepted();

        default:
            return QObject::event(e);
    }
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

QSize Component::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, 0), qMax(hhint, 0));
}

void Component::validate ()
{
    // nothing by default
}

void Component::draw (DrawContext* ctx) const
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

void Component::keyPressEvent (QKeyEvent* e)
{
    if (e->key() == Qt::Key_Tab && _focused) {

    } else {
        e->ignore(); // pass up to parent
    }
}

void Component::keyReleaseEvent (QKeyEvent* e)
{
    e->ignore(); // pass up to parent
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

void Container::removeChild (Component* child)
{
    if (_children.removeOne(child)) {
        dirty(child->bounds());
        child->setParent(0);
        invalidate();
    }
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

void Container::draw (DrawContext* ctx) const
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

void DrawContext::fillRect (int x, int y, int width, int height, int ch)
{
    // zero implies transparent
    if (ch == 0) {
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
        const int* sptr = contents;

        // if we're opaque, we can just copy line-by-line; otherwise, we must check for zero values
        if (opaque) {
            for (int yy = isect.height(); yy > 0; yy--) {
                qCopy(sptr, sptr + width, ldptr);
                sptr += width;
                ldptr += stride;
            }
        } else {
            int ch;
            for (int yy = isect.height(); yy > 0; yy--) {
                for (int* dptr = ldptr, *end = ldptr + isect.width(); dptr < end; dptr++) {
                    if ((ch = *sptr++) != 0) {
                        *dptr = ch;
                    }
                }
                ldptr += stride;
            }
        }
    }
}

void DrawContext::drawString (int x, int y, const QString& string, int style)
{
    // apply offset
    x += _pos.x();
    y += _pos.y();

    // check against each rect
    QRect draw(x, y, string.length(), 1);
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        QRect isect = rect.intersected(draw);
        if (isect.isEmpty()) {
            continue;
        }
        int stride = rect.width();
        int* dptr = _buffers[ii].data() + (isect.y() - rect.y())*stride + (isect.x() - rect.x());
        for (const QChar* sptr = string.constData(), *end = sptr + string.length();
                sptr < end; sptr++) {
            *dptr++ = (*sptr).unicode() | style;
        }
    }
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
