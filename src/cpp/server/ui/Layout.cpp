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

    QSize size = container->bounds().size();
    int nheight = 0, sheight = 0;
    if (comps[NORTH] != 0) {
        QSize pref = comps[NORTH]->preferredSize(size.width(), -1);
        comps[NORTH]->setBounds(QRect(0, 0, size.width(), nheight = pref.height()));
    }
    if (comps[SOUTH] != 0) {
        QSize pref = comps[SOUTH]->preferredSize(size.width(), -1);
        sheight = pref.height();
        comps[SOUTH]->setBounds(QRect(0, size.height() - sheight, size.width(), sheight));
    }

    int mheight = size.height() - nheight - sheight;
    int wwidth = 0, ewidth = 0;
    if (comps[WEST] != 0) {
        QSize pref = comps[WEST]->preferredSize(-1, mheight);
        comps[WEST]->setBounds(QRect(0, nheight, wwidth = pref.width(), mheight));
    }
    if (comps[EAST] != 0) {
        QSize pref = comps[EAST]->preferredSize(-1, mheight);
        ewidth = pref.width();
        comps[EAST]->setBounds(QRect(size.width() - ewidth, nheight, ewidth, mheight));
    }

    if (comps[CENTER] != 0) {
        comps[CENTER]->setBounds(QRect(wwidth, nheight, size.width() - wwidth - ewidth, mheight));
    }
}
