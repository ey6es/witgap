//
// $Id$

#ifndef WINDOW
#define WINDOW

#include <QRegion>

#include "ui/Component.h"

/**
 * A top-level window.
 */
class Window : public Container, protected DrawContext
{
    Q_OBJECT

public:

    /**
     * Initializes the window.
     */
    Window (QObject* parent, int layer = 0);

    /**
     * Destroys the window.
     */
    virtual ~Window ();

    /**
     * Returns the window's unique identifier.
     */
    int id () const { return _id; }

    /**
     * Sets the window's layer.
     */
    void setLayer (int layer);

    /**
     * Returns the window's layer.
     */
    int layer () const { return _layer; }

    /**
     * Sets the window's modal flag.
     */
    void setModal (bool modal);

    /**
     * Returns the window's modal flag.
     */
    bool modal () const { return _modal; }

    /**
     * Resizes the window to its preferred size.
     */
    void pack ();

    /**
     * Centers the window on the screen.
     */
    void center ();

    /**
     * Causes the window's state and contents to be resent (called on reconnect).
     */
    void resend();

public slots:

    /**
     * Invalidates the component.
     */
    virtual void invalidate ();

    /**
     * Dirties the component.
     */
    virtual void dirty (const QRect& region);

protected slots:

    /**
     * Notes that the window will require an update.
     */
    void noteNeedsUpdate ();

    /**
     * Enqueues a sync if one is not already enqueued.
     */
    void maybeEnqueueSync ();

protected:

    /**
     * Updates the state of the window and synchronizes the client's state with it.
     */
    Q_INVOKABLE void sync ();

    /**
     * Returns the meta-method for {@link #sync}.
     */
    static const QMetaMethod& syncMetaMethod ();

    /** The window's id. */
    int _id;

    /** The window's layer. */
    int _layer;

    /** Whether or not the window is modal. */
    bool _modal;

    /** Whether or not a sync is currently enqueued. */
    bool _syncEnqueued;

    /** Whether or not the window is up-to-date. */
    bool _upToDate;
};

#endif // WINDOW
