//
// $Id$

#ifndef LAYOUT
#define LAYOUT

#include <QObject>
#include <QSize>

class Container;

/**
 * Represents the layout of a component.
 */
class Layout
{
public:

    /**
     * Default constructor.
     */
    Layout ();

    /**
     * Computes the preferred size of the specified container.
     */
    virtual QSize computePreferredSize (
        const Container* container, int whint, int hhint) const = 0;

    /**
     * Applies the layout to the specified container.
     */
    virtual void apply (Container* container) const = 0;

private:

    Q_DISABLE_COPY(Layout)
};

/**
 * Arranges components from left-to-right or from top-to-bottom.
 */
class BoxLayout
{
public:

    /**
     * Creates a new box layout.
     */
    BoxLayout (Qt::Orientation orientation = Qt::Horizontal, int gap = 5);

    /**
     * Computes the preferred size of the specified container.
     */
    virtual QSize computePreferredSize (
        const Container* container, int whint, int hhint) const;

    /**
     * Applies the layout to the specified container.
     */
    virtual void apply (Container* container) const;

protected:

    /** The box orientation. */
    Qt::Orientation _orientation;

    /** The gap between components. */
    int _gap;
};

/**
 * Arranges edges components around a central one.
 */
class BorderLayout
{
public:

    /**
     * Position constants.
     */
    enum Position { NORTH, EAST, SOUTH, WEST, CENTER };

    /**
     * Computes the preferred size of the specified container.
     */
    virtual QSize computePreferredSize (
        const Container* container, int whint, int hhint) const;

    /**
     * Applies the layout to the specified container.
     */
    virtual void apply (Container* container) const;
};

#endif // LAYOUT
