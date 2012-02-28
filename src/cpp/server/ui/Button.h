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

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return true; }

signals:

    /**
     * Emitted when the button is pressed.
     */
    void pressed ();

public slots:

    /**
     * Invalidates the button.
     */
    virtual void invalidate ();

protected:

    /**
     * Handles a focus in event.
     */
    virtual void focusInEvent (QFocusEvent* e);

    /**
     * Handles a focus out event.
     */
    virtual void focusOutEvent (QFocusEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);
};

#endif // BUTTON
