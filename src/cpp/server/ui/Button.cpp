//
// $Id$

#include "Protocol.h"
#include "ui/Button.h"

Button::Button (const QIntVector& text, Qt::Alignment alignment, QObject* parent) :
    Label(text, alignment, parent)
{
    updateMargins();
}

void Button::doPress ()
{
    if (!_focused) {
        requestFocus();
    }
    emit pressed();
}

void Button::invalidate ()
{
    Component::invalidate();
    setTextFlags((_enabled ? 0 : DIM_FLAG) | (_focused ? REVERSE_FLAG : 0),
        ~(DIM_FLAG | REVERSE_FLAG));
}

void Button::updateMargins ()
{
    Label::updateMargins();

    // add space for brackets
    _margins.setLeft(_margins.left() + 1);
    _margins.setRight(_margins.right() + 1);
}

void Button::draw (DrawContext* ctx) const
{
    Label::draw(ctx);

    // draw the brackets
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(_margins.left() - 1, _margins.top(), '[' | flags);
    ctx->drawChar(_bounds.width() - _margins.right(), _margins.top(), ']' | flags);
}

void Button::focusInEvent (QFocusEvent* e)
{
    setTextFlag(REVERSE_FLAG, true);
}

void Button::focusOutEvent (QFocusEvent* e)
{
    setTextFlag(REVERSE_FLAG, false);
}

void Button::mouseButtonReleaseEvent (QMouseEvent* e)
{
    if (localBounds().contains(e->pos())) {
        emit pressed();
    }
}

void Button::keyPressEvent (QKeyEvent* e)
{
    QString text = e->text();
    QChar ch = text.isEmpty() ? 0 : text.at(0);
    if (ch.isPrint() || ch == '\r') {
        emit pressed();

    } else {
        Label::keyPressEvent(e);
    }
}

CheckBox::CheckBox (
        const QIntVector& text, bool selected, Qt::Alignment alignment, QObject* parent) :
    Button(text, alignment, parent),
    _selected(selected)
{
    connect(this, SIGNAL(pressed()), SLOT(toggleSelected()));
    updateMargins();
}

void CheckBox::setSelected (bool selected)
{
    if (_selected != selected) {
        _selected = selected;
        dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
    }
}

void CheckBox::invalidate ()
{
    Label::invalidate();
}

void CheckBox::updateMargins ()
{
    Label::updateMargins();

    // add space for check indicator
    _margins.setLeft(_margins.left() + 4);
}

void CheckBox::draw (DrawContext* ctx) const
{
    Label::draw(ctx);

    // draw the indicator
    int flags = _enabled ? 0 : DIM_FLAG;
    ctx->drawChar(_margins.left() - 4, _margins.top(), '[' | flags);
    ctx->drawChar(_margins.left() - 3, _margins.top(),
        (_selected ? 'X' : ' ') | (_focused ? REVERSE_FLAG : 0) | flags);
    ctx->drawChar(_margins.left() - 2, _margins.top(), ']' | flags);
}

void CheckBox::focusInEvent (QFocusEvent* e)
{
    dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
}

void CheckBox::focusOutEvent (QFocusEvent* e)
{
    dirty(QRect(_margins.left() - 3, _margins.top(), 1, 1));
}
