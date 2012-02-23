//
// $Id$

#ifndef BORDER
#define BORDER

#include <QMargins>
#include <QObject>
#include <QVector>

#include "util/General.h"

class DrawContext;

/**
 * Represents a border around a component.
 */
class Border
{
public:

    /**
     * Default constructor.
     */
    Border ();

    /**
     * Destroys the border.
     */
    virtual ~Border ();

    /**
     * Returns the border's margins.
     */
    virtual QMargins margins () const = 0;

    /**
     * Draws the border.
     */
    virtual void draw (DrawContext* ctx, int x, int y, int width, int height) const = 0;

private:

    Q_DISABLE_COPY(Border)
};

/**
 * A border with arbitrary margins that repeats a single character.
 */
class CharBorder : public Border
{
public:

    /**
     * Creates a new character border.
     */
    CharBorder (QMargins margins = QMargins(1, 1, 1, 1), int character = ' ');

    /**
     * Returns the border's margins.
     */
    virtual QMargins margins () const;

    /**
     * Draws the border.
     */
    virtual void draw (DrawContext* ctx, int x, int y, int width, int height) const;

protected:

    /** The border margins. */
    QMargins _margins;

    /** The border character. */
    int _character;
};

/**
 * A border that uses a set of characters to create a frame.
 */
class FrameBorder : public Border
{
public:

    /**
     * Creates a new frame border.
     */
    FrameBorder (
        int n = '-', int ne = '+', int e = '|', int se = '+',
        int s = '-', int sw = '+', int w = '|', int nw = '+');

    /**
     * Returns the border's margins.
     */
    virtual QMargins margins () const;

    /**
     * Draws the border.
     */
    virtual void draw (DrawContext* ctx, int x, int y, int width, int height) const;

protected:

    /** The border characters. */
    int _n, _ne, _e, _se, _s, _sw, _w, _nw;
};

/**
 * A border with a title.
 */
class TitledBorder : public FrameBorder
{
public:

    /**
     * Creates a new titled border.
     */
    TitledBorder (
        const QIntVector& title, Qt::Alignment alignment = Qt::AlignHCenter,
        int n = '-', int ne = '+', int e = '|', int se = '+', int s = '-',
        int sw = '+', int w = '|', int nw = '+');

    /**
     * Draws the border.
     */
    virtual void draw (DrawContext* ctx, int x, int y, int width, int height) const;

protected:

    /** The title contents. */
    QIntVector _title;

    /** The preferred alignment. */
    Qt::Alignment _alignment;
};

/**
 * A border that nests some number of contained sub-borders.
 */
class CompoundBorder : public Border
{
public:

    /**
     * Creates a new compound border.
     */
    CompoundBorder (
        Border* b0 = 0, Border* b1 = 0, Border* b2 = 0, Border* b3 = 0,
        Border* b4 = 0, Border* b5 = 0, Border* b6 = 0, Border* b7 = 0);

    /**
     * Destroys the border.
     */
    virtual ~CompoundBorder ();

    /**
     * Returns the border's margins.
     */
    virtual QMargins margins () const;

    /**
     * Draws the border.
     */
    virtual void draw (DrawContext* ctx, int x, int y, int width, int height) const;

protected:

    /** The component borders. */
    QVector<Border*> _borders;
};

#endif // BORDER
