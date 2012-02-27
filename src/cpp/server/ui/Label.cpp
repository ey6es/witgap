//
// $Id$

#include <limits>

#include "Protocol.h"
#include "ui/Label.h"

using namespace std;

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
    // scan for line breaks
    int length = 0, maxLength = 0, lines = 1;
    int wlimit = (whint == -1) ? numeric_limits<int>::max() : whint;
    const int* whitespace = 0;
    bool wrun = false;
    for (const int* ptr = _text.constData(), *end = ptr + _text.size(); ptr < end; ptr++) {
        int value = getChar(*ptr & 0xFFFF);
        if (value != '\n') {
            if (value == ' ') {
                if (!wrun) { // it's the start of a run of whitespace
                    wrun = true;
                    whitespace = ptr;
                }
            } else {
                wrun = false;
            }
            if (++length <= wlimit) {
                continue;
            }
            if (whitespace == 0) { // nowhere to break
                length--;
                ptr--;

            } else {
                length -= (ptr - whitespace + 1);

                // consume any whitespace after the line
                for (ptr = whitespace + 1; ptr < end && getChar(*ptr) == ' '; ptr++);
                ptr--;
            }
        }
        maxLength = qMax(maxLength, length);
        length = 0;
        whitespace = 0;
        wrun = false;
        lines++;
    }
    maxLength = qMax(maxLength, length);

    return QSize(qMax(whint, maxLength), qMax(hhint, lines));
}

void Label::validate ()
{
    // find the line breaks
    int length = 0;
    int wlimit = innerRect().width();
    const int* start = _text.constData(), *whitespace = 0;
    bool wrun = false;
    _lines.resize(0);
    for (const int* ptr = start, *end = ptr + _text.size(); ptr < end; ptr++) {
        int value = getChar(*ptr & 0xFFFF);
        if (value != '\n') {
            if (value == ' ') {
                if (!wrun) { // it's the start of a run of whitespace
                    wrun = true;
                    whitespace = ptr;
                }
            } else {
                wrun = false;
            }
            if (++length <= wlimit) {
                continue;
            }
            if (whitespace == 0) { // nowhere to break
                length--;

            } else {
                length -= (ptr - whitespace + 1);

                // consume any whitespace after the line
                for (ptr = whitespace + 1; ptr < end && getChar(*ptr) == ' '; ptr++);
            }
            ptr--;
        }
        _lines.append(Line(start, length));
        start = ptr + 1;
        length = 0;
        whitespace = 0;
        wrun = false;
    }
    _lines.append(Line(start, length));
}

void Label::draw (DrawContext* ctx) const
{
    Component::draw(ctx);

    // find the area within the margins
    QRect inner = innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();

    // get the alignment
    int halign = _alignment & Qt::AlignHorizontal_Mask;
    int valign = _alignment & Qt::AlignVertical_Mask;

    // draw the lines
    if (valign == Qt::AlignVCenter) {
        y += (height - _lines.size()) / 2;
    } else if (valign == Qt::AlignBottom) {
        y += (height - _lines.size());
    }
    foreach (const Line& pair, _lines) {
        int lx = x;
        if (halign == Qt::AlignHCenter) {
            lx += (width - pair.second)/2;
        } else if (halign == Qt::AlignRight) {
            lx += (width - pair.second);
        }
        ctx->drawContents(lx, y, pair.second, 1, pair.first);
        y++;
    }
}
