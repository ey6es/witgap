//
// $Id$

#include "ui/Border.h"

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

FrameBorder::FrameBorder (int n, int ne, int e, int se, int s, int sw, int w, int nw) :
    _n(n), _ne(ne), _e(e), _se(se), _s(s), _sw(sw), _w(w), _nw(nw)
{
}

QMargins FrameBorder::margins () const
{
    return QMargins(1, 1, 1, 1);
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
