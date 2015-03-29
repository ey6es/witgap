//
// $Id$

#include <QKeyEvent>
#include <QtDebug>

#include "ui/ScrollPane.h"

ScrollPane::ScrollPane (Component* component, Policy policy, QObject* parent) :
    Component(parent),
    _component(0),
    _policy(policy)
{
    setComponent(component);
}

void ScrollPane::setComponent (Component* component, bool destroyPrevious)
{
    if (_component != 0) {
        if (destroyPrevious) {
            delete _component;
        } else {
            _component->setParent(0);
        }
    }
    if ((_component = component) != 0) {
        _component->setParent(this);
    }
    invalidate();
    Component::dirty(innerRect());
    _scrollAmount = QPoint(0, 0);
}

void ScrollPane::setPolicy (Policy policy)
{
    if (_policy != policy) {
        _policy = policy;
        invalidate();
    }
}

void ScrollPane::setPosition (const QPoint& position)
{
    QPoint clamped;
    QRect inner = innerRect();
    if (_component != 0) {
        clamped = QPoint(
            qBound(0, position.x(), _component->bounds().width() - inner.width()),
            qBound(0, position.y(), _component->bounds().height() - inner.height()));
    }
    if (_position != clamped) {
        // detect scrolling
        QSize size = inner.size();
        if (QRect(_position, size).intersects(QRect(clamped, size))) {
            QPoint delta = _position - clamped;
            _scrollAmount += delta;
            scrollDirty(delta);

        } else {
            _scrollAmount = QPoint(0, 0);
            Component::dirty(inner);
        }
        _position = clamped;
    }
}

QPoint ScrollPane::basePos () const
{
    return absolutePos() - _position;
}

void ScrollPane::ensureShowing (const QRect& rect)
{
    QRect current = innerRect();
    current.translate(_position);
    if (current.contains(rect)) {
        return;
    }
    if (_policy.testFlag(HScroll)) {
        if (rect.left() < current.left()) {
            current.moveLeft(rect.left());

        } else if (rect.right() > current.right()) {
            current.moveRight(rect.right());
        }
    }
    if (_policy.testFlag(VScroll)) {
        if (rect.top() < current.top()) {
            current.moveTop(rect.top());

        } else if (rect.bottom() > current.bottom()) {
            current.moveBottom(rect.bottom());
        }
    }
    setPosition(QPoint(current.left() - _margins.left(), current.top() - _margins.top()));
}

Component* ScrollPane::componentAt (QPoint pos, QPoint* relative)
{
    if (_component != 0 && (innerRect() &= _component->bounds()).contains(pos)) {
        Component* comp = _component->componentAt(
            pos - _component->bounds().topLeft() + _position, relative);
        if (comp != 0) {
            return comp;
        }
    }
    return Component::componentAt(pos, relative);
}

Component* ScrollPane::transferFocus (Component* from, Direction dir)
{
    if (_component == 0) {
        return Component::transferFocus(from, dir);
    }
    if (!(_enabled && _visible)) {
        return 0;
    }
    return (from == 0) ? _component->transferFocus(0, dir) : Component::transferFocus(this, dir);
}

void ScrollPane::dirty ()
{
    // when we dirty the entire component, we do not want it relative to the position
    Component::dirty(QRect(0, 0, _bounds.width(), _bounds.height()));
    _scrollAmount = QPoint(0, 0);
}

void ScrollPane::dirty (const QRect& region)
{
    // get region relative to position and clip
    QRect rregion = region.translated(-_position) &= innerRect();
    if (!rregion.isEmpty()) {
        Component::dirty(rregion);
    }
}

QSize ScrollPane::computePreferredSize (int whint, int hhint) const
{
    if (_component == 0) {
        return Component::computePreferredSize(whint, hhint);
    }
    QSize csize = _component->preferredSize(
        _policy.testFlag(HScroll) ? -1 : whint,
        _policy.testFlag(VScroll) ? -1 : hhint);
    return QSize(
        (whint >= 0 && _policy.testFlag(HScroll)) ? whint : csize.width(),
        (hhint >= 0 && _policy.testFlag(VScroll)) ? hhint : csize.height());
}

void ScrollPane::validate ()
{
    if (_component == 0) {
        return;
    }
    QRect inner = innerRect();
    QSize psize = _component->preferredSize(
        _policy.testFlag(HScroll) ? -1 : inner.width(),
        _policy.testFlag(VScroll) ? -1 : inner.height());
    _component->setBounds(QRect(inner.topLeft(), QSize(
        _policy.testFlag(HScroll) ? psize.width() : qMin(psize.width(), inner.width()),
        _policy.testFlag(VScroll) ? psize.height() : qMin(psize.height(), inner.height()))));
    _component->maybeValidate();

    setPosition(_position); // reclamp the position
}

void ScrollPane::draw (DrawContext* ctx)
{
    Component::draw(ctx);

    if (_component == 0) {
        return;
    }

    // apply scroll, if any
    QRect inner = innerRect();
    if (_scrollAmount != QPoint(0, 0)) {
        ctx->scrollContents(inner, _scrollAmount);
        _scrollAmount = QPoint(0, 0);
    }

    QPoint cpos = ctx->pos();
    inner.translate(cpos);
    _dirty = ctx->dirty().intersected(inner);
    prepareForDrawing();

    // draw the component
    _pos = inner.topLeft() - _position;
    fillRect(_position.x(), _position.y(), inner.width(), inner.height(), 0, true);
    _component->maybeDraw(this);

    // copy the contents
    ctx->untranslate(cpos);
    for (int ii = 0, nn = _rects.size(); ii < nn; ii++) {
        const QRect& rect = _rects.at(ii);
        ctx->drawContents(rect.left(), rect.top(), rect.width(), rect.height(),
            _buffers.at(ii).constData(), false);
    }
    ctx->translate(cpos);
}

void ScrollPane::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier || _component == 0) {
        Component::keyPressEvent(e);
        return;
    }
    QRect inner = innerRect();
    switch (e->key()) {
        case Qt::Key_PageUp:
            setPosition(QPoint(_position.x(), _position.y() - inner.height()));
            break;

        case Qt::Key_PageDown:
            setPosition(QPoint(_position.x(), _position.y() + inner.height()));
            break;

        case Qt::Key_Home:
            setPosition(QPoint(0, 0));
            break;

        case Qt::Key_End:
            setPosition(QPoint(_component->bounds().width() - inner.width(),
                _component->bounds().height() - inner.height()));
            break;

        default:
            Component::keyPressEvent(e);
            break;
    }
}

void ScrollPane::moveContents (const QRect& source, const QPoint& dest)
{
    qWarning() << "Move contents not supported for scroll pane.";
}
