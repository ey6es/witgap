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

protected:

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /** The location of the view. */
    QPoint _location;
};

#endif // SCENE_VIEW
