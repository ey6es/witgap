//
// $Id$

#ifndef LABEL
#define LABEL

#include <QBasicTimer>
#include <QPair>
#include <QVector>

#include "ui/Component.h"

class IntVector;

/**
 * A text label.
 */
class Label : public Component
{
    Q_OBJECT

public:

    /**
     * Initializes the label.
     */
    Label (const QIntVector& text = QIntVector(),
        Qt::Alignment alignment = Qt::AlignLeft | Qt::AlignTop, QObject* parent = 0);

    /**
     * Sets the label text.
     */
    void setText (const QIntVector& text);

    /**
     * Returns the label text.
     */
    const QIntVector& text () const { return _text; }

    /**
     * Sets the text alignment.
     */
    void setAlignment (Qt::Alignment alignment);

    /**
     * Returns the text alignment.
     */
    Qt::Alignment alignment () const { return _alignment; }

    /**
     * Sets or clears a set of flags for all characters in the text.
     */
    void setTextFlag (int flag, bool set = true) { setTextFlags(set ? flag : 0, ~flag); }

    /**
     * Sets the flags for all characters in the text.
     */
    void setTextFlags (int flags, int mask = 0xFFFF);

    /**
     * Enables or disables the component.
     */
    virtual void setEnabled (bool enabled);

public slots:

    /**
     * Invalidates the label.
     */
    virtual void invalidate ();

protected:

    /** The start position and length of a line. */
    typedef QPair<const int*, int> Line;

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
    virtual void draw (DrawContext* ctx);

    /** The label text. */
    QIntVector _text;

    /** The text alignment. */
    Qt::Alignment _alignment;

    /** The location and length of each line. */
    QVector<Line> _lines;
};

/**
 * A label used for status updates.
 */
class StatusLabel : public Label
{
    Q_OBJECT

public:

    /**
     * Initializes the status label.
     */
    StatusLabel (const QIntVector& text = QIntVector(),
        Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignVCenter, QObject* parent = 0);

    /**
     * Sets the label text, optionally flashing to draw attention to it.
     */
    void setStatus (const QIntVector& text, bool flash = false);

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
     * Handles a timer event.
     */
    virtual void timerEvent (QTimerEvent* e);

    /** The timer that we use for flashes. */
    QBasicTimer _timer;

    /** The number of flashes remaining, if flashing. */
    int _countdown;
};

#endif // LABEL
