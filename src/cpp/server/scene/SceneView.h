//
// $Id$

#ifndef SCENE_VIEW
#define SCENE_VIEW

#include <QPoint>

#include "ui/Component.h"

class Scene;

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
    SceneView (Session* session);

    /**
     * Destroys the view.
     */
    virtual ~SceneView ();

    /**
     * Sets the bounds of the view in world space.
     */
    void setWorldBounds (const QRect& bounds);

    /**
     * Returns a reference to the bounds of the view in world space.
     */
    const QRect& worldBounds () const { return _worldBounds; };

public slots:

    /**
     * Adds the view's spatial representation to the scene.
     */
    void addSpatial (Scene* scene);

    /**
     * Removes the view's spatial representation from the scene.
     */
    void removeSpatial (Scene* scene);

    /**
     * Invalidates the component.
     */
    virtual void invalidate ();

protected:

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /** The bounds of the view in world space. */
    QRect _worldBounds;
};

#endif // SCENE_VIEW
