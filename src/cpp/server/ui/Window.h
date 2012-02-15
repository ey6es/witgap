//
// $Id$

#ifndef WINDOW
#define WINDOW

#include "ui/Component.h"

/**
 * A top-level window.
 */
class Window : public Component
{
    Q_OBJECT

public:

    /**
     * Initializes the window.
     */
    Window (QObject* parent, int id, int layer = 0);

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

protected:

    /** The window's id. */
    int _id;

    /** The window's layer. */
    int _layer;
};

#endif // WINDOW
