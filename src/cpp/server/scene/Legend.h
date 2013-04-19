//
// $Id$

#ifndef LEGEND
#define LEGEND

#include "scene/Scene.h"
#include "ui/Component.h"

/**
 * Displays the set of relevant labels.
 */
class Legend : public Container
{
    Q_OBJECT

public:
    
    /**
     * Initializes the legend.
     */
    Legend (Session* session);

    /**
     * Notes that a label has changed within the view bounds.
     *
     * @param npos the position to which the label will be moving, or zero for none.
     */
    void labelChanged (LabelPointer olabel, LabelPointer nlabel, const QPoint* npos = 0);

protected slots:
    
    /**
     * Notes that we just entered a scene.
     */
    void handleDidEnterScene (Scene* scene);

    /**
     * Notes that we're about to leave a scene.
     */
    void handleWillLeaveScene (Scene* scene);
    
    /**
     * Updates the label set, taking into account the intersection between old and new bounds.
     */
    void update (const QRect& oldBounds);
    
protected:

    /**
     * Represents a single active label.
     */    
    class LabelMapping
    {
    public:
        
        /** The number of visible instances of the label. */
        int count;
        
        /** The label component. */
        Component* component; 
    };
    
    /**
     * Adds the labels intersecting the specified bounds from the set.
     */
    void add (const QRect& bounds);

    /**
     * Subtracts the labels intersecting the specified bounds from the set.
     */
    void subtract (const QRect& bounds);

    /** The set of active labels. */
    QHash<LabelPointer, LabelMapping> _labels;
};

#endif // LEGEND
