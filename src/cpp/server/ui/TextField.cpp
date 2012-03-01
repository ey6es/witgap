//
// $Id$

#include "Protocol.h"
#include "ui/TextField.h"

TextField::TextField (int width, const QString& text, QObject* parent) :
    Component(parent),
    _width(width),
    _documentPos(0),
    _cursorPos(0)
{
}

QSize TextField::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, _width + 2), qMax(hhint, 1));
}

void TextField::validate ()
{

}

void TextField::draw (DrawContext* ctx) const
{
    Component::draw(ctx);

    // find the area within the margins
    QRect inner = innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();

    // center within allocated area
    x += (width - _width - 2) / 2;
    y += (height - 1) / 2;

    // draw the borders
    ctx->drawChar(x, y, '[');
    ctx->drawChar(x + _width + 1, y, ']');
}

void TextField::focusInEvent (QFocusEvent* e)
{
    dirty();
}

void TextField::focusOutEvent (QFocusEvent* e)
{
    dirty();
}

void TextField::mouseButtonPressEvent (QMouseEvent* e)
{
    Component::mouseButtonPressEvent(e);
}

void TextField::keyPressEvent (QKeyEvent* e)
{
    Component::keyPressEvent(e);
}
