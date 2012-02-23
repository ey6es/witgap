//
// $Id$

#include <QRect>

#include "ui/Component.h"
#include "ui/Layout.h"

Layout::Layout ()
{
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
    return QSize(1, 1);
}

void TableLayout::apply (Container* container) const
{
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
    QSize size = container->bounds().size();
    const QMargins& margins = container->margins();
    int x = margins.left(), y = margins.top();
    int width = size.width() - (margins.left() + margins.right());
    int height = size.height() - (margins.top() + margins.bottom());

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
