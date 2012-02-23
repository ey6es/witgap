//
// $Id$

#include <QRect>

#include "ui/Component.h"
#include "ui/Layout.h"

Layout::Layout ()
{
}

BoxLayout::BoxLayout (Qt::Orientation orientation, int gap) :
    _orientation(orientation),
    _gap(gap)
{
}

QSize BoxLayout::computePreferredSize (const Container* container, int whint, int hhint) const
{
    return QSize(1, 1);
}

void BoxLayout::apply (Container* container) const
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
    if (comps[NORTH] != 0) {
        north = comps[NORTH]->preferredSize(whint, -1);
    }
    if (comps[SOUTH] != 0) {
        south = comps[SOUTH]->preferredSize(whint, -1);
    }
    if (hhint != -1) {
        hhint -= (north.height() + south.height());
    }

    if (comps[EAST] != 0) {
        east = comps[EAST]->preferredSize(-1, hhint);
    }
    if (comps[WEST] != 0) {
        west = comps[WEST]->preferredSize(-1, hhint);
    }
    if (whint != -1) {
        whint -= (east.width() + west.width());
    }

    if (comps[CENTER] != 0) {
        center = comps[CENTER]->preferredSize(whint, hhint);
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
    if (comps[NORTH] != 0) {
        QSize pref = comps[NORTH]->preferredSize(width, -1);
        comps[NORTH]->setBounds(QRect(x, y, width, nheight = pref.height()));
    }
    if (comps[SOUTH] != 0) {
        QSize pref = comps[SOUTH]->preferredSize(width, -1);
        sheight = pref.height();
        comps[SOUTH]->setBounds(QRect(x, y + height - sheight, width, sheight));
    }

    int mheight = height - nheight - sheight;
    int wwidth = 0, ewidth = 0;
    if (comps[WEST] != 0) {
        QSize pref = comps[WEST]->preferredSize(-1, mheight);
        comps[WEST]->setBounds(QRect(x, y + nheight, wwidth = pref.width(), mheight));
    }
    if (comps[EAST] != 0) {
        QSize pref = comps[EAST]->preferredSize(-1, mheight);
        ewidth = pref.width();
        comps[EAST]->setBounds(QRect(x + width - ewidth, y + nheight, ewidth, mheight));
    }

    if (comps[CENTER] != 0) {
        comps[CENTER]->setBounds(QRect(x + wwidth, y + nheight, width - wwidth - ewidth, mheight));
    }
}
