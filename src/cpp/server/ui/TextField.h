//
// $Id$

#ifndef TEXT_FIELD
#define TEXT_FIELD

#include "ui/Component.h"

/**
 * A text field.
 */
class TextField : public Component
{
    Q_OBJECT

public:

    /**
     * Creates a new text field.
     */
    TextField (QObject* parent = 0);
};

#endif // TEXT_FIELD
