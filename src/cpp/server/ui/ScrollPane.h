//
// $Id$

#ifndef SCROLL_PANE
#define SCROLL_PANE

#include "ui/Component.h"

/**
 * Provides a scrolling view of a component.
 */
class ScrollPane : public Component, protected DrawContext
{
    Q_OBJECT

public:

    /**
     * Policy flags.
     */
    enum PolicyFlag { NoScroll = 0x0, HScroll = 0x1, VScroll = 0x2 };

    Q_DECLARE_FLAGS(Policy, PolicyFlag)

    /**
     * Creates a new scroll pane.
     */
    ScrollPane (Component* component = 0, Policy policy = VScroll, QObject* parent = 0);

    /**
     * Sets the component over which we scroll.
     *
     * @param destroyPrevious if true, delete the previous component.
     */
    void setComponent (Component* component, bool destroyPrevious = true);

    /**
     * Returns a pointer to the component over which we scroll.
     */
    Component* component () const { return _component; }

    /**
     * Sets the scroll policy.
     */
    void setPolicy (Policy policy);

    /**
     * Returns the scroll policy.
     */
    Policy policy () const { return _policy; }

    /**
     * Sets the scroll position.
     */
    void setPosition (const QPoint& position);

    /**
     * Returns a reference to the scroll position.
     */
    const QPoint& position () const { return _position; }

    /**
     * Returns the base position to use when calculating the absolute position of children.
     */
    virtual QPoint basePos () const;

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
    virtual Component* transferFocus (Component* from, Direction dir);

    /**
     * Dirties the entire component.
     */
    virtual void dirty ();

    /**
     * Marks the specified region as dirty.
     */
    virtual void dirty (const QRect& region);

protected:

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
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Moves part of the contents.
     */
    virtual void moveContents (const QRect& source, const QPoint& dest);

    /** The component over which we scroll. */
    Component* _component;

    /** The scroll policy. */
    Policy _policy;

    /** The position in the component. */
    QPoint _position;

    /** The amount scrolled since the last render. */
    QPoint _scrollAmount;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ScrollPane::Policy)

#endif // SCROLL_PANE
