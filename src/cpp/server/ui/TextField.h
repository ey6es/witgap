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
    TextField (int width = 20, const QString& text = "", QObject* parent = 0);

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return true; }

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Validates the component.
     */
    virtual void validate ();

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /**
     * Handles a focus in event.
     */
    virtual void focusInEvent (QFocusEvent* e);

    /**
     * Handles a focus out event.
     */
    virtual void focusOutEvent (QFocusEvent* e);

    /**
     * Handles a mouse press event.
     */
    virtual void mouseButtonPressEvent (QMouseEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /** The width of the field. */
    int _width;

    /** The position in the document. */
    int _documentPos;

    /** The cursor position. */
    int _cursorPos;
};

#endif // TEXT_FIELD
