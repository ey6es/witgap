//
// $Id$

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>

#include "Protocol.h"
#include "ui/ScrollingList.h"

ScrollingList::ScrollingList (int minHeight, const QStringList& values, QObject* parent) :
    Component(parent),
    _minHeight(minHeight),
    _values(values),
    _listPos(0),
    _selectedIdx(-1)
{
}

void ScrollingList::setValues (const QStringList& values)
{
    _values = values;
    _listPos = 0;
    _selectedIdx = -1;
    invalidate();
    Component::dirty();
}

void ScrollingList::addValue (const QString& value, int idx)
{
    _values.insert(idx, value);
    dirty(idx);
}

void ScrollingList::removeValue (int idx)
{
    _values.removeAt(idx);
    dirty(idx);
}

void ScrollingList::setSelectedIndex (int index)
{
    if (_selectedIdx != index) {
        int oidx = _selectedIdx;
        _selectedIdx = index;
        if (!updateListPos()) {
            if (_selectedIdx != -1) {
                dirty(_selectedIdx, 1);
            }
            if (oidx != -1) {
                dirty(oidx, 1);
            }
        }
        emit selectionChanged();
    }
}

QSize ScrollingList::computePreferredSize (int whint, int hhint) const
{
    int maxLength = 0;
    foreach (const QString& value, _values) {
        maxLength = qMax(maxLength, value.length());
    }
    return QSize(qMax(whint, maxLength + 2), qMax(hhint, _minHeight));
}

void ScrollingList::draw (DrawContext* ctx)
{
    Component::draw(ctx);

    QRect inner = innerRect();
    int x = inner.x() + 1, y = inner.y(), width = inner.width() - 2, height = inner.height();

    for (int ii = _listPos, nn = _values.length(), yymax = y + height;
            ii < nn && y < yymax; ii++, y++) {
        const QString& value = _values.at(ii);
        int length = qMin(width, value.length());
        if (ii == _selectedIdx) {
            ctx->drawChar(x - 1, y, '[');
            if (_focused) {
                ctx->drawString(x, y, value.constData(), length, REVERSE_FLAG);
                if (width > length) {
                    ctx->fillRect(x + length, y, width - length, 1, ' ' | REVERSE_FLAG);
                }
            } else {
                ctx->drawString(x, y, value.constData(), length);
            }
            ctx->drawChar(x + width, y, ']');
        } else {
            ctx->drawString(x, y, value.constData(), length);
        }
    }
}

void ScrollingList::focusInEvent (QFocusEvent* e)
{
    if (_selectedIdx == -1) {
        if (!_values.isEmpty()) {
            setSelectedIndex(_listPos);
        }
    } else {
        dirty(_selectedIdx, 1);
    }
}

void ScrollingList::focusOutEvent (QFocusEvent* e)
{
    if (_selectedIdx != -1) {
        dirty(_selectedIdx, 1);
    }
}

void ScrollingList::mouseButtonPressEvent (QMouseEvent* e)
{
    QRect inner = innerRect();
    inner.setX(inner.x() + 1);
    inner.setWidth(inner.width() - 1);
    if (inner.contains(e->pos())) {
        int idx = _listPos + e->pos().y() - inner.y();
        if (idx < _values.length()) {
            setSelectedIndex(idx);
        }
    }
    Component::mouseButtonPressEvent(e);
}

void ScrollingList::keyPressEvent (QKeyEvent* e)
{
    Qt::KeyboardModifiers modifiers = e->modifiers();
    if (modifiers != Qt::ShiftModifier && modifiers != Qt::NoModifier || _selectedIdx == -1) {
        Component::keyPressEvent(e);
        return;
    }
    switch (e->key()) {
        case Qt::Key_Up:
            if (_selectedIdx > 0) {
                _selectedIdx--;
                if (!updateListPos()) {
                    dirty(_selectedIdx, 2);
                }
                emit selectionChanged();

            } else {
                Component::keyPressEvent(e);
            }
            break;

        case Qt::Key_Down:
            if (_selectedIdx < _values.length() - 1) {
                _selectedIdx++;
                if (!updateListPos()) {
                    dirty(_selectedIdx - 1, 2);
                }
                emit selectionChanged();

            } else {
                Component::keyPressEvent(e);
            }
            break;

        case Qt::Key_PageUp:
            setSelectedIndex(qMax(0, _selectedIdx - listAreaHeight()));
            break;

        case Qt::Key_PageDown:
            setSelectedIndex(qMin(_values.length() - 1, _selectedIdx + listAreaHeight()));
            break;

        case Qt::Key_Home:
            setSelectedIndex(0);
            break;

        case Qt::Key_End:
            setSelectedIndex(_values.length() - 1);
            break;

        case Qt::Key_Return:
        case Qt::Key_Enter:
            emit enterPressed();
            break;

        default:
            Component::keyPressEvent(e);
            break;
    }
}

bool ScrollingList::updateListPos ()
{
    if (_selectedIdx == -1) {
        return false;
    }
    int height = listAreaHeight();
    int opos = _listPos;
    _listPos = qBound(qMax(_selectedIdx - height + 1, 0), _listPos,
        qMin(_selectedIdx, _values.length() - height));
    if (opos != _listPos) {
        Component::dirty();
        return true;
    }
    return false;
}

void ScrollingList::dirty (int idx)
{
    // dirty the lesser of the remaining list (plus one for a
    // possibly deleted entry) and what's visible
    int llength = _values.length();
    int height = listAreaHeight();
    dirty(idx, qMin(llength - idx + 1, height - idx + _listPos));
}

void ScrollingList::dirty (int idx, int length)
{
    Component::dirty(QRect(_margins.left(), _margins.top() + idx - _listPos,
        _bounds.width() - _margins.left() - _margins.right(), length));
}

int ScrollingList::listAreaHeight () const
{
    return _bounds.height() - _margins.top() - _margins.bottom();
}
