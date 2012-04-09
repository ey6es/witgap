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
     * Notes that we just entered a scene.
     */
    void handleDidEnterScene (Scene* scene);

    /**
     * Notes that we're about to leave a scene.
     */
    void handleWillLeaveScene (Scene* scene);

    /**
     * Scrolls, if necessary, in response to the pawn's moving.
     */
    void maybeScroll ();

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
