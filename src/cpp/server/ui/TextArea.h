//
// $Id$

#ifndef TEXT_AREA
#define TEXT_AREA

#include "ui/Component.h"

/**
 * Allows editing text consisting of multiple lines.
 */
class TextArea : public Component
{
    Q_OBJECT

public:
    
    /**
     * Creates a new text area.
     */
    TextArea (QObject* parent = 0);
};

#endif // TEXT_AREA
