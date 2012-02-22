//
// $Id$

#ifndef WINDOW
#define WINDOW

#include <QRegion>

#include "ui/Component.h"

class Session;

/**
 * A top-level window.
 */
class Window : public Container, public DrawContext
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
     * Returns a pointer to the session that owns the window.
     */
    Session* session () const;

    /**
     * Returns the window's unique identifier.
     */
    int id () const { return _id; }

    /**
     * Returns the window's layer.
     */
    int layer () const { return _layer; }

    /**
     * Sets the window's layer.
     */
    void setLayer (int layer);

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

    /** Whether or not a sync is currently enqueued. */
    bool _syncEnqueued;

    /** Whether or not the window is added. */
    bool _added;

    /** Whether or not the window is up-to-date. */
    bool _upToDate;
};

#endif // WINDOW
