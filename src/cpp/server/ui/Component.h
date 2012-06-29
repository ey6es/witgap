//
// $Id$

#ifndef COMPONENT
#define COMPONENT

#include <QList>
#include <QMargins>
#include <QRect>
#include <QRegion>
#include <QSize>
#include <QVariant>
#include <QVector>

#include "util/Callback.h"
#include "util/General.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QTimerEvent;

class Border;
class Container;
class DrawContext;
class Layout;
class Session;
class Window;

/**
 * Base class of all user interface components.
 */
class Component : public CallableObject
{
    Q_OBJECT

public:

    /** Directions for focus transfer. */
    enum Direction { NoDirection, Forward, Backward, Left, Right, Up, Down };

    /**
     * Initializes the component.
     */
    Component (QObject* parent = 0);

    /**
     * Destroys the component.
     */
    virtual ~Component ();

    /**
     * Returns the parent of this component, if it is a container.
     */
    Container* container () const { return qobject_cast<Container*>(parent()); }

    /**
     * Returns a pointer to the window that contains the component.
     */
    Window* window ();

    /**
     * Returns a pointer to the session that owns the component.
     */
    Session* session () const;

    /**
     * Sets the component's bounds in its parent coordinate system.
     */
    void setBounds (const QRect& bounds);

    /**
     * Returns the component's bounds in its parent coordinate system.
     */
    const QRect& bounds () const { return _bounds; }

    /**
     * Returns the component's margins.
     */
    const QMargins& margins () const { return _margins; }

    /**
     * Returns the component's bounds in its local coordinate system.
     */
    QRect localBounds () const;

    /**
     * Returns the inner rectangle (the bounds minus the margins) in the component's local
     * coordinate system.
     */
    QRect innerRect () const;

    /**
     * Returns the base position to use when calculating the absolute position of children.
     */
    virtual QPoint basePos () const { return absolutePos(); }

    /**
     * Returns the component's absolute position.
     */
    QPoint absolutePos () const;

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
     * Enables or disables the component.
     */
    virtual void setEnabled (bool enabled);

    /**
     * Checks whether the component is enabled.
     */
    bool enabled () const { return _enabled; }

    /**
     * Renders the component visible or invisible.
     */
    virtual void setVisible (bool visible);

    /**
     * Checks whether the component is visible.
     */
    bool visible () const { return _visible; }

    /**
     * Validates the component if necessary.
     */
    void maybeValidate ();

    /**
     * Draws the component if necessary.
     */
    void maybeDraw (DrawContext* ctx);

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return false; }

    /**
     * Ensures that the specified rectangle is showing (i.e., is scrolled into view).
     */
    virtual void ensureShowing (const QRect& rect);

    /**
     * Finds the component at the specified coordinates and populates the supplied
     * point with the relative coordinates.  Returns 0 if there isn't a component
     * at the coordinates.
     */
    virtual Component* componentAt (QPoint pos, QPoint* relative);

    /**
     * Attempts to transfer focus from the source component to the next one in the specified
     * direction.
     *
     * @param from the component to search from, or 0 to start from the beginning/end.
     * @return whether or not focus was successfully transferred.
     */
    virtual bool transferFocus (Component* from, Direction dir);

    /**
     * Handles an event.
     */
    virtual bool event (QEvent* e);

signals:

    /**
     * Emitted when the component's bounds have changed.
     */
    void boundsChanged ();

public slots:

    /**
     * Requests input focus for this component.
     */
    void requestFocus ();

    /**
     * Attempts to transfer focus from this component to the next.
     */
    void transferFocus () { transferFocus(this, Forward); }

    /**
     * Invalidates the component.
     */
    virtual void invalidate ();

    /**
     * Dirties the entire component.
     */
    virtual void dirty () { dirty(QRect(0, 0, _bounds.width(), _bounds.height())); }

    /**
     * Marks the specified region as dirty.
     */
    virtual void dirty (const QRect& region);

    /**
     * Scrolls the entire component by the specified delta.
     */
    void scrollDirty (const QPoint& delta);

    /**
     * Scrolls a section of the dirty region.
     */
    virtual void scrollDirty (const QRect& region, const QPoint& delta);

protected:

    /**
     * Updates the margins after changing the border, etc.
     */
    virtual void updateMargins ();

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Validates the component.
     */
    virtual void validate ();

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx);

    /**
     * Handles a focus in event.
     */
    virtual void focusInEvent (QFocusEvent* e);

    /**
     * Handles a focus out event.
     */
    virtual void focusOutEvent (QFocusEvent* e);

    /**
     * Handles a mouse press event.
     */
    virtual void mouseButtonPressEvent (QMouseEvent* e);

    /**
     * Handles a mouse release event.
     */
    virtual void mouseButtonReleaseEvent (QMouseEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Handles a key release event.
     */
    virtual void keyReleaseEvent (QKeyEvent* e);

    /**
     * Handles a timer event.
     */
    virtual void timerEvent (QTimerEvent* e);

    /**
     * Invalidates the component's parent, if it has one.
     */
    void invalidateParent () const;

    /** The bounds of the component. */
    QRect _bounds;

    /** The margins of the component (determined by its border). */
    QMargins _margins;

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

    /** If true, this component owns the input focus. */
    bool _focused;

    /** Whether or not the component is enabled. */
    bool _enabled;

    /** Whether or not the component is visible. */
    bool _visible;
};

/**
 * A component that simply takes up space.
 */
class Spacer : public Component
{
    Q_OBJECT

public:

    /**
     * Initializes the spacer.
     */
    Spacer (int width = -1, int height = -1, int background = 0x0, QObject* parent = 0);
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
    Container (Layout* layout = 0, QObject* parent = 0);

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
     *
     * @param destroy if true, delete the child.
     */
    void removeChild (Component* child, bool destroy = true);

    /**
     * Removes the child at the specified index.
     *
     * @param destroy if true, delete the child.
     */
    void removeChildAt (int idx, bool destroy = true);

    /**
     * Removes all of the container's children.
     *
     * @param destroy if true, delete the child.
     */
    void removeAllChildren (bool destroy = true);

    /**
     * Returns the container's child list.
     */
    const QList<Component*>& children () const { return _children; }

    /**
     * Returns the number of visible children in this container.
     */
    int visibleChildCount () const;

    /**
     * Finds the component at the specified coordinates and populates the supplied
     * point with the relative coordinates.  Returns 0 if there isn't a component
     * at the coordinates.
     */
    virtual Component* componentAt (QPoint pos, QPoint* relative);

    /**
     * Attempts to transfer focus from the source component to the next one in the specified
     * direction.
     *
     * @param from the component to search from, or 0 to start from the beginning/end.
     * @return whether or not focus was successfully transferred.
     */
    virtual bool transferFocus (Component* from, Direction dir);

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Validates the container.
     */
    virtual void validate ();

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx);

    /** The container's layout, if any. */
    Layout* _layout;

    /** The children of the container. */
    QList<Component*> _children;
};

/**
 * The context object passed to components for drawing.
 */
class DrawContext
{
public:

    /**
     * Translates the position.
     */
    void translate (const QPoint& offset) { _pos += offset; }

    /**
     * Untranslates the position.
     */
    void untranslate (const QPoint& offset) { _pos -= offset; }

    /**
     * Returns a reference to the position.
     */
    const QPoint& pos () const { return _pos; }

    /**
     * Returns a reference to the dirty region.
     */
    const QRegion& dirty () const { return _dirty; }

    /**
     * Determines whether the dirty region intersects the specified rectangle.
     */
    bool isDirty (const QRect& rect) const { return _dirty.intersects(rect.translated(_pos)); }

    /**
     * Draws a single character.
     */
    void drawChar (int x, int y, int ch);

    /**
     * Fills a rectangle with the supplied character.
     *
     * @param force if true, fill the rectangle even if the character is zero.
     */
    void fillRect (int x, int y, int width, int height, int ch, bool force = false);

    /**
     * Draws the supplied contents.
     *
     * @param opaque if false, check for zero values and don't draw them.
     */
    void drawContents (
        int x, int y, int width, int height, const int* contents, bool opaque = true);

    /**
     * Draws a string.
     */
    void drawString (int x, int y, const QString& string, int style = 0);

    /**
     * Draws a string.
     */
    void drawString (int x, int y, const QChar* string, int length, int style = 0);

    /**
     * Scrolls part of the contents.
     */
    void scrollContents (const QRect& rect, const QPoint& amount);

    /**
     * Moves part of the contents.
     */
    void moveContents (int x, int y, int width, int height, int dx, int dy);

protected:

    /**
     * Prepares the context for drawing.
     */
    void prepareForDrawing ();

    /**
     * Moves part of the contents.
     */
    virtual void moveContents (const QRect& source, const QPoint& dest) = 0;

    /** The draw position. */
    QPoint _pos;

    /** The dirty region. */
    QRegion _dirty;

    /** The rectangles of which the dirty region is comprised. */
    QVector<QRect> _rects;

    /** The buffers holding the contents of the regions. */
    QVector<QIntVector> _buffers;
};

#endif // COMPONENT
