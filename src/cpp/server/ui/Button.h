//
// $Id$

#ifndef BUTTON
#define BUTTON

#include "ui/Label.h"

/**
 * A button.
 */
class Button : public Label
{
    Q_OBJECT

public:

    /**
     * Initializes the button.
     */
    Button (const QIntVector& text = QIntVector(), Qt::Alignment alignment = Qt::AlignLeft,
        QObject* parent = 0);
};

#endif // BUTTON
