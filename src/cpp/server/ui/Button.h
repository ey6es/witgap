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
     * Updates the margins after changing the border, etc.
     */
    virtual void updateMargins ();

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
     * Handles a mouse release event.
     */
    virtual void mouseButtonReleaseEvent (QMouseEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);
};

/**
 * A check box.
 */
class CheckBox : public Button
{
    Q_OBJECT

public:

    /**
     * Initializes the box.
     */
    CheckBox (const QIntVector& text = QIntVector(), bool selected = false,
        Qt::Alignment alignment = Qt::AlignLeft, QObject* parent = 0);

    /**
     * Selects or deselects the box.
     */
    void setSelected (bool selected);

    /**
     * Checks whether the box is selected.
     */
    bool selected () const { return _selected; }

public slots:

    /**
     * Invalidates the box.
     */
    virtual void invalidate ();

    /**
     * Toggles the selected state.
     */
    void toggleSelected () { setSelected(!_selected); }

protected:

    /**
     * Updates the margins after changing the border, etc.
     */
    virtual void updateMargins ();

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

    /** Whether or not the box is selected. */
    bool _selected;
};

#endif // BUTTON
