//
// $Id$

#ifndef COMPONENT
#define COMPONENT

#include <QList>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QVariant>

class Border;
class Layout;

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
     * Destroys the component.
     */
    virtual ~Component ();

    /**
     * Sets the component's bounds.
     */
    void setBounds (const QRect& bounds);

    /**
     * Returns the component's bounds.
     */
    const QRect& bounds () const { return _bounds; }

    /**
     * Sets the component's border.  The component will assume ownership of the object.
     */
    void setBorder (Border* border);

    /**
     * Returns the component's border.
     */
    Border* border () const { return _border; }

    /**
     * Sets the component's background.
     */
    void setBackground (int background);

    /**
     * Returns the component's background.
     */
    int background () const { return _background; }

    /**
     * Sets the component's preferred size.  Either or both dimensions may be -1 to indicate that
     * they should be computed.
     */
    void setPreferredSize (const QSize& size);

    /**
     * Returns the component's preferred size.
     */
    QSize preferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Sets the component's layout constraint.
     */
    void setConstraint (const QVariant& constraint);

    /**
     * Returns the component's layout constraint.
     */
    const QVariant& constraint () const { return _constraint; }

    /**
     * Validates the component if necessary.
     */
    void maybeValidate ();

signals:

    /**
     * Emitted when the component's bounds have changed.
     */
    void boundsChanged ();

public slots:

    /**
     * Invalidates the component.
     */
    virtual void invalidate ();

protected:

    /**
     * Validates the component.
     */
    virtual void validate ();

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Invalidates the component's parent, if it has one.
     */
    void invalidateParent () const;

    /** The bounds of the component. */
    QRect _bounds;

    /** The component's border, if any. */
    Border* _border;

    /** The explicit preferred size (either or both dimensions may be -1 to
     * indicate that they should be computed). */
    QSize _explicitPreferredSize;

    /** The background character. */
    int _background;

    /** The component's layout constraint within its container. */
    QVariant _constraint;

    /** If false, this component is invalid and must be layed out again. */
    bool _valid;
};

/**
 * A component that contains other components.
 */
class Container : public Component
{
    Q_OBJECT

public:

    /**
     * Initializes the container.
     */
    Container (QObject* parent = 0);

    /**
     * Destroys the container.
     */
    virtual ~Container ();

    /**
     * Sets the container's layout.  The container will assume ownership of the object.
     */
    void setLayout (Layout* layout);

    /**
     * Returns the container's layout.
     */
    Layout* layout () const { return _layout; }

    /**
     * Adds a child to the container.
     */
    void addChild (Component* child, const QVariant& constraint = QVariant());

    /**
     * Removes a child from the container.
     */
    void removeChild (Component* child);

    /**
     * Returns the container's child list.
     */
    const QList<Component*>& children () const { return _children; }

protected:

    /**
     * Validates the container.
     */
    virtual void validate ();

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /** The container's layout, if any. */
    Layout* _layout;

    /** The children of the container. */
    QList<Component*> _children;
};

#endif // COMPONENT
