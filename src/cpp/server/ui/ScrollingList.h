//
// $Id$

#ifndef SCROLLING_LIST
#define SCROLLING_LIST

#include <QStringList>

#include "ui/Component.h"

/**
 * Contains a scrolling list of selectable items.
 */
class ScrollingList : public Component
{
    Q_OBJECT

public:

    /**
     * Creates a new scrolling list.
     */
    ScrollingList (
        int minHeight = 5, const QStringList& values = QStringList(), QObject* parent = 0);

    /**
     * Sets the value list.
     */
    void setValues (const QStringList& values);

    /**
     * Returns a reference to the value list.
     */
    const QStringList& values () const { return _values; }

    /**
     * Adds a value to the list.
     */
    void addValue (const QString& value) { addValue(value, _values.length()); }

    /**
     * Adds a value to the list.
     */
    void addValue (const QString& value, int idx);

    /**
     * Removes a value from the list.
     */
    void removeValue (int idx);

    /**
     * Sets the index of the selected item.
     */
    void setSelectedIndex (int index);

    /**
     * Returns the index of the selected item.
     */
    int selectedIndex () const { return _selectedIdx; }

    /**
     * Checks whether the component accepts input focus.
     */
    virtual bool acceptsFocus () const { return _enabled && _visible; }

signals:

    /**
     * Emitted when the selection has changed.
     */
    void selectionChanged ();

    /**
     * Emitted when the enter key is pressed.
     */
    void enterPressed ();

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /**
     * Handles a mouse press event.
     */
    virtual void mouseButtonPressEvent (QMouseEvent* e);

    /**
     * Handles a key press event.
     */
    virtual void keyPressEvent (QKeyEvent* e);

    /**
     * Updates the list position to match the selected index.
     *
     * @return whether or not the component was dirtied as a result of the update.
     */
    bool updateListPos ();

    /**
     * Dirties the region starting at the supplied list index and including everything after.
     */
    void dirty (int idx);

    /**
     * Dirties the region corresponding to the described list section.
     */
    void dirty (int idx, int length);

    /**
     * Returns the height of the actual list area.
     */
    int listAreaHeight () const;

    /** The minimum height. */
    int _minHeight;

    /** The values to display. */
    QStringList _values;

    /** The position in the list. */
    int _listPos;

    /** The index of the selected item, or -1 for none. */
    int _selectedIdx;
};

#endif // SCROLLING_LIST
