//
// $Id$

#ifndef COMPONENT
#define COMPONENT

#include <QObject>
#include <QRect>

/**
 * Base class of all user interface components.
 */
class Component : public QObject
{
    Q_OBJECT

public:

    /**
     * Initializes the component.
     */
    Component (QObject* parent = 0);

    /**
     * Returns the component's bounds.
     */
    QRect bounds () const { return _bounds; }

    /**
     * Sets the component's bounds.
     */
    void setBounds (const QRect& bounds);

protected:

    /** The bounds of the component. */
    QRect _bounds;
};

#endif // COMPONENT
