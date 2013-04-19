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
    ctx->fillRect(x, y, width, _margins.top(), _character);
    ctx->fillRect(x + width - _margins.right(), y + _margins.top(), _margins.right(),
        height - _margins.top() - _margins.bottom(), _character);
    ctx->fillRect(x, y + height - _margins.bottom(), width, _margins.bottom(), _character);
    ctx->fillRect(x, y + _margins.top(), _margins.left(),
        height - _margins.top() - _margins.bottom(), _character);
}

FrameBorder::FrameBorder (int n, int ne, int e, int se, int s, int sw, int w, int nw) :
    _n(n), _ne(ne), _e(e), _se(se), _s(s), _sw(sw), _w(w), _nw(nw)
{
}

QMargins FrameBorder::margins () const
{
    return QMargins(
        (_nw == -1 && _w == -1 && _sw == -1) ? 0 : 1,
        (_nw == -1 && _n == -1 && _ne == -1) ? 0 : 1,
        (_ne == -1 && _e == -1 && _se == -1) ? 0 : 1,
        (_sw == -1 && _s == -1 && _se == -1) ? 0 : 1);
}

void FrameBorder::draw (DrawContext* ctx, int x, int y, int width, int height) const
{
    QMargins margins = this->margins();
    int right = x + width - 1;
    int bottom = y + height - 1;

    if (_n != -1) {
        ctx->fillRect(x + margins.left(), y, width - margins.left() - margins.right(), 1, _n);
    }
    if (_ne != -1) {
        ctx->drawChar(right, y, _ne);
    }
    if (_e != -1) {
        ctx->fillRect(right, y + margins.top(), 1, height - margins.top() - margins.bottom(), _e);
    }
    if (_se != -1) {
        ctx->drawChar(right, bottom, _se);
    }
    if (_s != -1) {
        ctx->fillRect(x + margins.left(), bottom, width - margins.left() - margins.right(), 1, _s);
    }
    if (_sw != -1) {
        ctx->drawChar(x, bottom, _sw);
    }
    if (_w != -1) {
        ctx->fillRect(x, y + margins.top(), 1, height - margins.top() - margins.bottom(), _w);
    }
    if (_nw != -1) {
        ctx->drawChar(x, y, _nw);
    }
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
