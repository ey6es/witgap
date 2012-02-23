//
// $Id$

#include "ui/Label.h"

Label::Label (const QIntVector& text, Qt::Alignment alignment, QObject* parent) :
    Component(parent),
    _text(text),
    _alignment(alignment)
{
}

void Label::setText (const QIntVector& text)
{
    if (_text != text) {
        _text = text;
        invalidate();
        dirty();
    }
}

void Label::setAlignment (Qt::Alignment alignment)
{
    if (_alignment != alignment) {
        _alignment = alignment;
        dirty();
    }
}

QSize Label::computePreferredSize (int whint, int hhint) const
{
    return QSize(qMax(whint, 0), qMax(hhint, 0));
}

void Label::draw (DrawContext* ctx) const
{
    Component::draw(ctx);


}
