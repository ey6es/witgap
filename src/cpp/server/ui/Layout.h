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
class BoxLayout : public Layout
{
public:

    /**
     * Policy flags.
     */
    enum PolicyFlag { NoPolicy = 0x0, HStretch = 0x1, VStretch = 0x2 };

    Q_DECLARE_FLAGS(Policy, PolicyFlag)

    /**
     * Constraints.
     */
    enum Constraint { Fixed = 1 };

    /**
     * Creates a new box layout.
     */
    BoxLayout (Qt::Orientation orientation = Qt::Horizontal, Policy policy = NoPolicy,
        Qt::Alignment alignment = Qt::AlignCenter, int gap = 1);

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

    /** The policy flags. */
    Policy _policy;

    /** The alignment flags. */
    Qt::Alignment _alignment;

    /** The gap between components. */
    int _gap;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(BoxLayout::Policy)

/**
 * Arranges components in a table.
 */
class TableLayout : public Layout
{
public:

    /**
     * Creates a new table layout.
     */
    TableLayout (int columns, int rows = -1, int columnGap = 1, int rowGap = 0);

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

    /** The number of columns, or -1 for any. */
    int _columns;

    /** The number of rows, or -1 for any. */
    int _rows;

    /** The gap between columns. */
    int _columnGap;

    /** The gap between rows. */
    int _rowGap;
};

/**
 * Arranges edges components around a central one.
 */
class BorderLayout : public Layout
{
public:

    /**
     * Position constants.
     */
    enum Position { North, East, South, West, Center };

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
