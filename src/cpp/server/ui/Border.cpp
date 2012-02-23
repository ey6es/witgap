//
// $Id$

#include "ui/Border.h"
#include "ui/Component.h"

Border::Border ()
{
}

Border::~Border ()
{
}

CharBorder::CharBorder (QMargins margins, int character) :
    _margins(margins),
    _character(character)
{
}

QMargins CharBorder::margins () const
{
    return _margins;
}

void CharBorder::draw (DrawContext* ctx, int x, int y, int width, int height) const
{
    ctx->fillRect(x, y, width, 1, _character);
    ctx->fillRect(x + width - 1, y + 1, 1, height - 2, _character);
    ctx->fillRect(x, y + height - 1, width, 1, _character);
    ctx->fillRect(x, y + 1, 1, height - 2, _character);
}

FrameBorder::FrameBorder (int n, int ne, int e, int se, int s, int sw, int w, int nw) :
    _n(n), _ne(ne), _e(e), _se(se), _s(s), _sw(sw), _w(w), _nw(nw)
{
}

QMargins FrameBorder::margins () const
{
    return QMargins(1, 1, 1, 1);
}

void FrameBorder::draw (DrawContext* ctx, int x, int y, int width, int height) const
{
    int right = x + width - 1;
    int bottom = y + height - 1;

    ctx->fillRect(x + 1, y, width - 2, 1, _n);
    ctx->drawChar(right, y, _ne);
    ctx->fillRect(right, y + 1, 1, height - 2, _e);
    ctx->drawChar(right, bottom, _se);
    ctx->fillRect(x + 1, bottom, width - 2, 1, _s);
    ctx->drawChar(x, bottom, _sw);
    ctx->fillRect(x, y + 1, 1, height - 2, _w);
    ctx->drawChar(x, y, _nw);
}

TitledBorder::TitledBorder (
        const QIntVector& title, Qt::Alignment alignment,
        int n, int ne, int e, int se, int s, int sw, int w, int nw) :
    FrameBorder(n, ne, e, se, s, sw, w, nw),
    _title(title),
    _alignment(alignment)
{
}

void TitledBorder::draw (DrawContext* ctx, int x, int y, int width, int height) const
{
    FrameBorder::draw(ctx, x, y, width, height);

    int length = qMin(_title.size(), width);
    if (_alignment == Qt::AlignRight) {
        x = width - length;
    } else if (_alignment == Qt::AlignHCenter) {
        x = (width - length) / 2;
    }
    ctx->drawContents(x, y, length, 1, _title.constData());
}

CompoundBorder::CompoundBorder (
    Border* b0, Border* b1, Border* b2, Border* b3, Border* b4, Border* b5, Border* b6, Border* b7)
{
    Border* borders[] = { b0, b1, b2, b3, b4, b5, b6, b7 };
    for (int ii = 0; ii < 8; ii++) {
        if (borders[ii] != 0) {
            _borders.append(borders[ii]);
        }
    }
}

CompoundBorder::~CompoundBorder ()
{
    foreach (Border* border, _borders) {
        delete border;
    }
}

QMargins CompoundBorder::margins () const
{
    int left = 0, top = 0, right = 0, bottom = 0;
    foreach (Border* border, _borders) {
        QMargins margins = border->margins();
        left += margins.left();
        top += margins.top();
        right += margins.right();
        bottom += margins.bottom();
    }
    return QMargins(left, top, right, bottom);
}

void CompoundBorder::draw (DrawContext* ctx, int x, int y, int width, int height) const
{
    foreach (Border* border, _borders) {
        QMargins margins = border->margins();
        x += margins.left();
        y += margins.top();
        width -= margins.left() + margins.right();
        height -= margins.top() + margins.bottom();

        border->draw(ctx, x, y, width, height);
    }
}
