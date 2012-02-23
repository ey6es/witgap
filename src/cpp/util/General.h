//
// $Id$

#ifndef GENERAL
#define GENERAL

#include <QString>
#include <QVector>

/**
 * A vector of integers.
 */
class QIntVector : public QVector<int>
{
public:

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
    QIntVector (const QString& string, int style = 0);
};

#endif // GENERAL
