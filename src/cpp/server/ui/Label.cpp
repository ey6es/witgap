//
// $Id$

#include <limits>

#include "Protocol.h"
#include "ui/Label.h"
#include "util/General.h"

using namespace std;

Label::Label (const QIntVector& text, Qt::Alignment alignment, Wrap wrap, QObject* parent) :
    Component(parent),
    _text(text),
    _alignment(alignment),
    _wrap(wrap)
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

void Label::setWrap (Wrap wrap)
{
    if (_wrap != wrap) {
        _wrap = wrap;
        invalidate();
        dirty();
    }
}

void Label::setTextFlags (int flags, int mask)
{
    for (int* ptr = _text.data(), *end = ptr + _text.size(); ptr < end; ptr++) {
        *ptr = (*ptr & mask) | flags;
    }
    dirty();
}

void Label::toggleTextFlags (int flags)
{
    for (int* ptr = _text.data(), *end = ptr + _text.size(); ptr < end; ptr++) {
        *ptr ^= flags;
    }
    dirty();
}

void Label::setEnabled (bool enabled)
{
    if (_enabled != enabled) {
        setTextFlag(DIM_FLAG, !(_enabled = enabled));
    }
}

void Label::invalidate ()
{
    Component::invalidate();
    setTextFlag(DIM_FLAG, !_enabled);
}

QSize Label::computePreferredSize (int whint, int hhint) const
{
    // scan for line breaks
    int length = 0, maxLength = 0, lines = 1;
    int wlimit = (whint == -1 || _wrap == NoWrap) ? numeric_limits<int>::max() : whint;
    const int* whitespace = 0;
    bool wrun = false;
    for (const int* ptr = _text.constData(), *end = ptr + _text.size(); ptr < end; ptr++) {
        int value = getChar(*ptr);
        if (value != '\n') {
            if (value == ' ' && _wrap == WordWrap) {
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
            if (whitespace == 0) { // break on character
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
    QRect inner = innerRect();
    int wlimit = (_wrap == NoWrap) ? numeric_limits<int>::max() : inner.width();
    const int* start = _text.constData(), *whitespace = 0;
    bool wrun = false;
    _lines.resize(0);
    for (const int* ptr = start, *end = ptr + _text.size(); ptr < end; ptr++) {
        int value = getChar(*ptr);
        if (value != '\n') {
            if (value == ' ' && _wrap == WordWrap) {
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
        appendLine(start, length);
        start = ptr + 1;
        length = 0;
        whitespace = 0;
        wrun = false;
    }
    appendLine(start, length);
}

void Label::draw (DrawContext* ctx)
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
    foreach (const QIntVector& line, _lines) {
        int lx = x, lwidth = line.size();
        if (halign == Qt::AlignHCenter) {
            lx += (width - lwidth)/2;
        } else if (halign == Qt::AlignRight) {
            lx += (width - lwidth);
        }
        ctx->drawContents(lx, y, lwidth, 1, line.constData());
        y++;
    }
}

void Label::appendLine (const int* start, int length)
{
    // if we exceed available height, replace last line with ellipsis
    QRect inner = innerRect();
    int nlines = _lines.size();
    if (nlines >= inner.height()) {
        if (nlines > 0) {
            _lines[nlines - 1] = QIntVector(qMin(3, inner.width()), '.');
        }
        return;
    }
    // likewise, if we exceed the width, replace end with ellipsis
    if (length > inner.width()) {
        QIntVector line(inner.width(), '.');
        length = qMax(inner.width() - 3, 0);
        qCopy(start, start + length, line.data());

        // copy the flags from the last character
        int flags = getFlags(*(start + qMax(0, length - 1)));
        for (int* ptr = line.data() + length, *end = line.data() + line.size(); ptr < end; ptr++) {
            *ptr |= flags;
        }
        _lines.append(line);
        return;
    }
    QIntVector line(length, 0);
    qCopy(start, start + length, line.data());
    _lines.append(line);
}

StatusLabel::StatusLabel (const QIntVector& text, Qt::Alignment alignment, QObject* parent) :
    Label(text, alignment, WordWrap, parent),
    _countdown(0)
{
    updateMargins();
}

void StatusLabel::setStatus (const QIntVector& text, bool flash)
{
    setText(text);
    if (flash) {
        // simulate a timer event for the first flash
        _countdown = 6;
        timerEvent(0);

        // then run every 250 ms afterwards
        _timer.start(400, this);
    }
}

void StatusLabel::updateMargins ()
{
    Label::updateMargins();

    // add space for highlights
    _margins.setLeft(_margins.left() + 1);
    _margins.setRight(_margins.right() + 1);
}

void StatusLabel::draw (DrawContext* ctx)
{
    Label::draw(ctx);

    // if the countdown is odd, show the flash
    if (_countdown % 2 == 1) {
        ctx->drawChar(_margins.left() - 1, _margins.top(), ' ' | REVERSE_FLAG);
        ctx->drawChar(_bounds.width() - _margins.right(), _margins.top(), ' ' | REVERSE_FLAG);
    }
}

void StatusLabel::timerEvent (QTimerEvent* e)
{
    if (--_countdown == 0) {
        _timer.stop();
    }
    dirty(QRect(_margins.left() - 1, _margins.top(), 1, 1));
    dirty(QRect(_bounds.width() - _margins.right(), _margins.top(), 1, 1));
}
