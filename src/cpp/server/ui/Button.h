//
// $Id$

#ifndef BUTTON
#define BUTTON

#include <QStringList>

#include "ui/Label.h"

class Menu;

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

/**
 * A button that brings up a menu of choices.
 */
class ComboBox : public Button
{
    Q_OBJECT

public:

    /**
     * Initializes the box.
     */
    ComboBox (const QStringList& items = QStringList(), Qt::Alignment alignment = Qt::AlignLeft,
        QObject* parent = 0);

    /**
     * Sets the list of items available for selection.
     */
    void setItems (const QStringList& items);

    /**
     * Returns a reference to the list of items available for selection.
     */
    const QStringList& items () const { return _items; }

    /**
     * Sets the selected index.
     */
    void setSelectedIndex (int index);

    /**
     * Returns the index of the currently selected item.
     */
    int selectedIndex () const { return _selectedIndex; }

    /**
     * Returns the selected item, or an invalid string for none.
     */
    QString selectedItem () const;

signals:

    /**
     * Fired when the selection changes.
     */
    void selectionChanged ();

protected slots:

    /**
     * Creates the popup menu for the box.
     */
    void createMenu ();

    /**
     * Called when a menu item is selected.
     */
    void selectItem ();

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /** The items available for selection. */
    QStringList _items;

    /** The index of the currently selected item. */
    int _selectedIndex;

    /** The selection menu, if showing. */
    Menu* _menu;
};

#endif // BUTTON
