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
    Button (const QString& label = QString(), Qt::Alignment alignment = Qt::AlignLeft,
        QObject* parent = 0);

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return _enabled && _visible; }

    /**
     * Sets the button label text.
     */
    void setLabel (const QString& label);

    /**
     * Returns a reference to the label text.
     */
    const QString& label () const { return _label; }

signals:

    /**
     * Emitted when the button is pressed.
     */
    void pressed ();

public slots:

    /**
     * Programmatically performs a button press.
     */
    void doPress ();

protected:

    /**
     * Updates the margins after changing the border, etc.
     */
    virtual void updateMargins ();

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx);

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

    /**
     * Updates the button text based on the label and state.
     */
    virtual void updateText ();

    /** The button label. */
    QString _label;
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
    CheckBox (const QString& label = QString(), bool selected = false,
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
    virtual void draw (DrawContext* ctx);

    /**
     * Handles a focus in event.
     */
    virtual void focusInEvent (QFocusEvent* e);

    /**
     * Handles a focus out event.
     */
    virtual void focusOutEvent (QFocusEvent* e);

    /**
     * Updates the button text based on the label and state.
     */
    virtual void updateText ();

    /** Whether or not the box is selected. */
    bool _selected;
};

#endif // BUTTON
