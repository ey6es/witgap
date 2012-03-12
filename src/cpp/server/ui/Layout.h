//
// $Id$

#ifndef LAYOUT
#define LAYOUT

#include <QObject>
#include <QSet>

#include "ui/Component.h"

class QSize;

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

    /**
     * Attempts to transfer focus in the specified direction.
     */
    virtual bool transferFocus (
        Container* container, Component* from, Component::Direction dir) const;

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
     * Convenience method to create a container with a horizontal box layout.
     */
    static Container* createHBox (
        int gap = 2, Component* c0 = 0, Component* c1 = 0, Component* c2 = 0, Component* c3 = 0,
        Component* c4 = 0, Component* c5 = 0, Component* c6 = 0, Component* c7 = 0);

    /**
     * Convenience method to create a container with a vertical box layout.
     */
    static Container* createVBox (
        int gap = 0, Component* c0 = 0, Component* c1 = 0, Component* c2 = 0, Component* c3 = 0,
        Component* c4 = 0, Component* c5 = 0, Component* c6 = 0, Component* c7 = 0);

    /**
     * Convenience method to create a container with a horizontal box layout that stretches
     * in both directions.
     */
    static Container* createHStretchBox (
        int gap = 2, Component* c0 = 0, Component* c1 = 0, Component* c2 = 0, Component* c3 = 0,
        Component* c4 = 0, Component* c5 = 0, Component* c6 = 0, Component* c7 = 0);

    /**
     * Convenience method to create a container with a vertical box layout that stretches
     * in both directions.
     */
    static Container* createVStretchBox (
        int gap = 0, Component* c0 = 0, Component* c1 = 0, Component* c2 = 0, Component* c3 = 0,
        Component* c4 = 0, Component* c5 = 0, Component* c6 = 0, Component* c7 = 0);

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
     * Returns a reference to the set of columns to stretch.
     */
    QSet<int>& stretchColumns () { return _stretchColumns; }

    /**
     * Returns a reference to the set of rows to stretch.
     */
    QSet<int>& stretchRows () { return _stretchRows; }

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

    /**
     * Given the child index, places the row and column into the specified locations.
     */
    void getRowAndColumn (int idx, int* row, int* column) const;

    /** The number of columns, or -1 for any. */
    int _columns;

    /** The number of rows, or -1 for any. */
    int _rows;

    /** The gap between columns. */
    int _columnGap;

    /** The gap between rows. */
    int _rowGap;

    /** Indices of columns to stretch. */
    QSet<int> _stretchColumns;

    /** Indices of rows to stretch. */
    QSet<int> _stretchRows;
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
