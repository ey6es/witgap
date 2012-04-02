//
// $Id$

#ifndef GENERAL
#define GENERAL

#include <QByteArray>
#include <QPoint>
#include <QVector>

class QString;

/**
 * A vector of integers.
 */
class QIntVector : public QVector<int>
{
public:

    /**
     * Creates a vector from the supplied string where any characters preceded by an ampersand are
     * highlighted.
     */
    static QIntVector createHighlighted (const QString& string);

    /**
     * Creates an empty vector.
     */
    QIntVector ();

    /**
     * Creates a vector with the supplied size.
     *
     * @param value the value with which to initialize the vector.
     */
    QIntVector (int size, int value = 0);

    /**
     * Creates a new vector from the supplied string.
     */
    QIntVector (const char* string, int style = 0);

    /**
     * Creates a new vector from the supplied string.
     */
    QIntVector (const QString& string, int style = 0);
};

/**
 * Hash function for points.
 */
inline uint qHash (const QPoint& point) { return point.x()*31 + point.y(); }

/**
 * Generates a random token of the specified length.
 */
QByteArray generateToken (int length);

#endif // GENERAL
