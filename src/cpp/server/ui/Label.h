//
// $Id$

#ifndef LABEL
#define LABEL

#include "ui/Component.h"
#include "util/General.h"

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
    Label (const QIntVector& text = QIntVector(), Qt::Alignment alignment = Qt::AlignLeft,
        QObject* parent = 0);

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

protected:

    /**
     * Computes and returns the preferred size.
     */
    virtual QSize computePreferredSize (int whint = -1, int hhint = -1) const;

    /**
     * Draws the component.
     */
    virtual void draw (DrawContext* ctx) const;

    /** The label text. */
    QIntVector _text;

    /** The text alignment. */
    Qt::Alignment _alignment;
};

#endif // LABEL
