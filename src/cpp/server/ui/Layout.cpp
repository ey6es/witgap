//
// $Id$

#include <numeric>

#include <QVarLengthArray>
#include <QtDebug>

#include "ui/Layout.h"

using namespace std;

Layout::Layout ()
{
}

bool Layout::transferFocus (
    Container* container, Component* from, Component::Direction dir) const
{
    // default behavior just translates right/down to forward and left/up to backward
    container->transferFocus(from, (dir == Component::Right || dir == Component::Down) ?
        Component::Forward : Component::Backward);
}

/**
 * Helper function for convenience factory functions: adds the arguments as children if
 * they are non-zero.
 */
static void addChildren (
    Container* container, Component* c0, Component* c1, Component* c2, Component* c3)
{
    Component* components[] = { c0, c1, c2, c3 };
    for (int ii = 0; ii < 4; ii++) {
        if (components[ii] != 0) {
            container->addChild(components[ii]);
        }
    }
}

Container* BoxLayout::createHBox (
    int gap, Component* c0, Component* c1, Component* c2, Component* c3)
{
    Container* container = new Container(
        new BoxLayout(Qt::Horizontal, NoPolicy, Qt::AlignCenter, gap));
    addChildren(container, c0, c1, c2, c3);
    return container;
}

Container* BoxLayout::createVBox (
    int gap, Component* c0, Component* c1, Component* c2, Component* c3)
{
    Container* container = new Container(
        new BoxLayout(Qt::Vertical, NoPolicy, Qt::AlignCenter, gap));
    addChildren(container, c0, c1, c2, c3);
    return container;
}

Container* BoxLayout::createHStretchBox (
    int gap, Component* c0, Component* c1, Component* c2, Component* c3)
{
    Container* container = new Container(
        new BoxLayout(Qt::Horizontal, HStretch | VStretch, Qt::AlignCenter, gap));
    addChildren(container, c0, c1, c2, c3);
    return container;
}

Container* BoxLayout::createVStretchBox (
    int gap, Component* c0, Component* c1, Component* c2, Component* c3)
{
    Container* container = new Container(
        new BoxLayout(Qt::Vertical, HStretch | VStretch, Qt::AlignCenter, gap));
    addChildren(container, c0, c1, c2, c3);
    return container;
}

BoxLayout::BoxLayout (
        Qt::Orientation orientation, Policy policy, Qt::Alignment alignment, int gap) :
    _orientation(orientation),
    _policy(policy),
    _alignment(alignment),
    _gap(gap)
{
}

QSize BoxLayout::computePreferredSize (const Container* container, int whint, int hhint) const
{
    int ncomps = container->children().size();
    int tgap = (ncomps > 1) ? (ncomps - 1) * _gap : 0;
    if (_orientation == Qt::Horizontal) {
        int nhhint = _policy.testFlag(VStretch) ? hhint : -1;
        int totalWidth = 0;
        int maxHeight = 0;
        foreach (Component* comp, container->children()) {
            QSize size = comp->preferredSize(-1, nhhint);
            totalWidth += size.width();
            maxHeight = qMax(maxHeight, size.height());
        }
        return QSize(qMax(whint, totalWidth + tgap), qMax(hhint, maxHeight));

    } else { // _orientation == Qt::Vertical
        int nwhint = _policy.testFlag(HStretch) ? whint : -1;
        int maxWidth = 0;
        int totalHeight = 0;
        foreach (Component* comp, container->children()) {
            QSize size = comp->preferredSize(nwhint, -1);
            maxWidth = qMax(maxWidth, size.width());
            totalHeight += size.height();
        }
        return QSize(qMax(whint, maxWidth), qMax(hhint, totalHeight + tgap));
    }
}

void BoxLayout::apply (Container* container) const
{
    // find the area within the margins
    QRect inner = container->innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();

    // if we have 16 children or fewer, we can keep their sizes on the stack
    const QList<Component*>& children = container->children();
    int ncomps = children.size();
    int tgap = (ncomps > 1) ? (ncomps - 1) * _gap : 0;
    QVarLengthArray<QSize, 16> psizes(ncomps);

    // gather bits we need for both orientations
    bool hstretch = _policy.testFlag(HStretch);
    bool vstretch = _policy.testFlag(VStretch);
    int halign = _alignment & Qt::AlignHorizontal_Mask;
    int valign = _alignment & Qt::AlignVertical_Mask;

    if (_orientation == Qt::Horizontal) {
        // get the preferred sizes and compute the total width
        int hhint = vstretch ? height : -1;
        int totalWidth = 0;
        int nnfixed = 0;
        for (int ii = 0; ii < ncomps; ii++) {
            Component* comp = children.at(ii);
            QSize size = comp->preferredSize(-1, hhint);
            psizes[ii] = size;
            totalWidth += size.width();
            if (comp->constraint().toInt() != Fixed) {
                nnfixed++;
            }
        }
        totalWidth += tgap;

        // adjust for horizontal stretching/alignment
        if (hstretch && nnfixed != 0) {
            // distribute the extra space amongst the non-fixed components
            int extra = (width - totalWidth) / nnfixed;
            int remainder = (width - totalWidth) % nnfixed;
            for (int ii = 0; ii < ncomps; ii++) {
                if (children.at(ii)->constraint().toInt() != Fixed) {
                    QSize& psize = psizes[ii];
                    psize.rwidth() += extra;
                    if (remainder-- > 0) {
                        psize.rwidth()++;
                    }
                }
            }
        } else {
            if (halign == Qt::AlignHCenter) {
                x += (width - totalWidth) / 2;
            } else if (halign == Qt::AlignRight) {
                x += width - totalWidth;
            }
        }

        // adjust for vertical stretching and alignment, apply
        for (int ii = 0; ii < ncomps; ii++) {
            const QSize& psize = psizes[ii];
            QRect bounds(x, y, psize.width(), height);
            if (!vstretch) {
                int cheight = psize.height();
                if (valign == Qt::AlignVCenter) {
                    bounds.setY(y + (height - cheight) / 2);
                } else if (valign == Qt::AlignBottom) {
                    bounds.setY(y + height - cheight);
                }
                bounds.setHeight(cheight);
            }
            children.at(ii)->setBounds(bounds);
            x += bounds.width() + _gap;
        }
    } else { // _orientation == Qt::Vertical
        // get the preferred sizes and compute the total height
        int whint = hstretch ? width : -1;
        int totalHeight = 0;
        int nnfixed = 0;
        for (int ii = 0; ii < ncomps; ii++) {
            Component* comp = children.at(ii);
            QSize size = comp->preferredSize(whint, -1);
            psizes[ii] = size;
            totalHeight += size.height();
            if (comp->constraint().toInt() != Fixed) {
                nnfixed++;
            }
        }
        totalHeight += tgap;

        // adjust for vertical stretching/alignment
        if (vstretch && nnfixed != 0) {
            // distribute the extra space amongst the non-fixed components
            int extra = (height - totalHeight) / nnfixed;
            int remainder = (height - totalHeight) % nnfixed;
            for (int ii = 0; ii < ncomps; ii++) {
                if (children.at(ii)->constraint().toInt() != Fixed) {
                    QSize& psize = psizes[ii];
                    psize.rheight() += extra;
                    if (remainder-- > 0) {
                        psize.rheight()++;
                    }
                }
            }
        } else {
            if (valign == Qt::AlignVCenter) {
                y += (height - totalHeight) / 2;
            } else if (valign == Qt::AlignBottom) {
                x += height - totalHeight;
            }
        }

        // adjust for horizontal stretching and alignment, apply
        for (int ii = 0; ii < ncomps; ii++) {
            const QSize& psize = psizes[ii];
            QRect bounds(x, y, width, psize.height());
            if (!hstretch) {
                int cwidth = psize.width();
                if (halign == Qt::AlignHCenter) {
                    bounds.setX(x + (width - cwidth) / 2);
                } else if (halign == Qt::AlignRight) {
                    bounds.setX(x + width - cwidth);
                }
                bounds.setWidth(cwidth);
            }
            children.at(ii)->setBounds(bounds);
            y += bounds.height() + _gap;
        }
    }
}

TableLayout::TableLayout (int columns, int rows, int columnGap, int rowGap) :
    _columns(columns),
    _rows(rows),
    _columnGap(columnGap),
    _rowGap(rowGap)
{
}

QSize TableLayout::computePreferredSize (const Container* container, int whint, int hhint) const
{
    const QList<Component*>& children = container->children();
    int ncomps = children.size();
    int ncols = (_columns == -1) ? (ncomps / _rows + (ncomps % _rows == 0 ? 0 : 1)) : _columns;
    int nrows = (_rows == -1) ? (ncomps / _columns + (ncomps % _columns == 0 ? 0 : 1)) : _rows;
    QVarLengthArray<int, 16> widths(ncols), heights(nrows);
    qFill(widths.data(), widths.data() + ncols, 0);
    qFill(heights.data(), heights.data() + nrows, 0);

    // find the maximum height of all rows and columns
    for (int ii = 0; ii < ncomps; ii++) {
        int row, column;
        getRowAndColumn(ii, &row, &column);
        QSize size = children.at(ii)->preferredSize(-1, -1);
        widths[column] = qMax(widths[column], size.width());
        heights[row] = qMax(heights[row], size.height());
    }

    // sum up the maximum dimensions and gaps
    int cgap = (ncols > 1) ? (ncols - 1) * _columnGap : 0;
    int rgap = (nrows > 1) ? (nrows - 1) * _rowGap : 0;
    return QSize(
        qMax(whint, accumulate(widths.data(), widths.data() + ncols, cgap)),
        qMax(hhint, accumulate(heights.data(), heights.data() + nrows, rgap)));
}

void TableLayout::apply (Container* container) const
{
    const QList<Component*>& children = container->children();
    int ncomps = children.size();
    int ncols = (_columns == -1) ? (ncomps / _rows + (ncomps % _rows == 0 ? 0 : 1)) : _columns;
    int nrows = (_rows == -1) ? (ncomps / _columns + (ncomps % _columns == 0 ? 0 : 1)) : _rows;
    QVarLengthArray<int, 16> widths(ncols), heights(nrows);
    qFill(widths.data(), widths.data() + ncols, 0);
    qFill(heights.data(), heights.data() + nrows, 0);

    // find the maximum height of all rows and columns
    for (int ii = 0; ii < ncomps; ii++) {
        int row, column;
        getRowAndColumn(ii, &row, &column);
        QSize size = children.at(ii)->preferredSize(-1, -1);
        widths[column] = qMax(widths[column], size.width());
        heights[row] = qMax(heights[row], size.height());
    }

    // find the area within the margins
    QRect inner = container->innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();

    // distribute any extra space
    int cgap = (ncols > 1) ? (ncols - 1) * _columnGap : 0;
    int rgap = (nrows > 1) ? (nrows - 1) * _rowGap : 0;
    int extraWidth = width - accumulate(widths.data(), widths.data() + ncols, cgap);
    int extraHeight = height - accumulate(heights.data(), heights.data() + nrows, rgap);

    // add to stretch columns, if any; otherwise, center horizontally
    if (extraWidth > 0) {
        int nscols = _stretchColumns.size();
        if (nscols > 0) {
            int per = extraWidth / nscols;
            int remainder = extraWidth % nscols;
            foreach (int col, _stretchColumns) {
                widths[col] += per + (remainder-- > 0 ? 1 : 0);
            }
        } else {
            x += extraWidth/2;
        }
    }

    // add to stretch rows, if any; otherwise, center vertically
    if (extraHeight > 0) {
        int nsrows = _stretchRows.size();
        if (nsrows > 0) {
            int per = extraHeight / nsrows;
            int remainder = extraHeight % nsrows;
            foreach (int row, _stretchRows) {
                heights[row] += per + (remainder-- > 0 ? 1 : 0);
            }
        } else {
            y += extraHeight/2;
        }
    }

    // position the children
    for (int ii = 0; ii < ncomps; ii++) {
        int row, column;
        getRowAndColumn(ii, &row, &column);
        children.at(ii)->setBounds(QRect(
            x + accumulate(widths.data(), widths.data() + column, column * _columnGap),
            y + accumulate(heights.data(), heights.data() + row, row * _rowGap),
            widths[column], heights[row]));
    }
}

void TableLayout::getRowAndColumn (int idx, int* row, int* column) const
{
    if (_columns == -1) {
        *row = idx % _rows;
        *column = idx / _rows;
    } else {
        *row = idx / _columns;
        *column = idx % _columns;
    }
}

/**
 * Helper function for BorderLayout; finds all of the border units and places them in the provided
 * array.
 */
static void findBorderComponents (const QList<Component*> children, Component* comps[])
{
    foreach (Component* comp, children) {
        bool ok;
        int idx = comp->constraint().toInt(&ok);
        if (ok && idx >= 0 && idx < 5) {
            comps[idx] = comp;
        }
    }
}

QSize BorderLayout::computePreferredSize (const Container* container, int whint, int hhint) const
{
    Component* comps[5] = { 0, 0, 0, 0, 0 };
    findBorderComponents(container->children(), comps);

    QSize north, south, east, west, center;
    if (comps[North] != 0) {
        north = comps[North]->preferredSize(whint, -1);
    }
    if (comps[South] != 0) {
        south = comps[South]->preferredSize(whint, -1);
    }
    if (hhint != -1) {
        hhint -= (north.height() + south.height());
    }

    if (comps[East] != 0) {
        east = comps[East]->preferredSize(-1, hhint);
    }
    if (comps[West] != 0) {
        west = comps[West]->preferredSize(-1, hhint);
    }
    if (whint != -1) {
        whint -= (east.width() + west.width());
    }

    if (comps[Center] != 0) {
        center = comps[Center]->preferredSize(whint, hhint);
    }

    return QSize(
        qMax(north.width(), qMax(south.width(), west.width() + center.width() + east.width())),
        north.height() + south.height() + qMax(west.height(),
            qMax(center.height(), east.height())));
}

void BorderLayout::apply (Container* container) const
{
    Component* comps[5] = { 0, 0, 0, 0, 0 };
    findBorderComponents(container->children(), comps);

    // find the area within the margins
    QRect inner = container->innerRect();
    int x = inner.x(), y = inner.y(), width = inner.width(), height = inner.height();

    // the north and south components will determine the height of the rest
    int nheight = 0, sheight = 0;
    if (comps[North] != 0) {
        QSize pref = comps[North]->preferredSize(width, -1);
        comps[North]->setBounds(QRect(x, y, width, nheight = pref.height()));
    }
    if (comps[South] != 0) {
        QSize pref = comps[South]->preferredSize(width, -1);
        sheight = pref.height();
        comps[South]->setBounds(QRect(x, y + height - sheight, width, sheight));
    }

    // the east and west components will determine the width of the center
    int mheight = height - nheight - sheight;
    int wwidth = 0, ewidth = 0;
    if (comps[West] != 0) {
        QSize pref = comps[West]->preferredSize(-1, mheight);
        comps[West]->setBounds(QRect(x, y + nheight, wwidth = pref.width(), mheight));
    }
    if (comps[East] != 0) {
        QSize pref = comps[East]->preferredSize(-1, mheight);
        ewidth = pref.width();
        comps[East]->setBounds(QRect(x + width - ewidth, y + nheight, ewidth, mheight));
    }

    if (comps[Center] != 0) {
        comps[Center]->setBounds(QRect(x + wwidth, y + nheight, width - wwidth - ewidth, mheight));
    }
}
