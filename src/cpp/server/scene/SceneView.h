//
// $Id$

#ifndef SCENE_VIEW
#define SCENE_VIEW

#include <QPoint>

#include "ui/Component.h"

/**
 * A client's view of a scene.
 */
class SceneView : public Component
{
    Q_OBJECT

public:

    /**
     * Initializes the view.
     */
    SceneView (QObject* parent = 0);

    /**
     * Returns a reference to the view location.
     */
    const QPoint& location () const { return _location; }

    /**
     * Returns the bounds of the view in world space.
     */
    QRect worldBounds () const { return QRect(_location, _bounds.size()); };

protected:

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /** The location of the view in world space. */
    QPoint _location;
};

#endif // SCENE_VIEW
