//
// $Id$

#ifndef GENERAL
#define GENERAL

#include <QByteArray>
#include <QObject>
#include <QPair>
#include <QPoint>
#include <QVector>

class QString;
class QTranslator;

/**
 * Provides a means to create an object on receipt of a signal.
 */
class Creator : public QObject
{
    Q_OBJECT

public:

    /**
     * Creates a new creator.
     */
    Creator (QObject* parent, const QMetaObject* metaObject,
        QGenericArgument val0 = QGenericArgument(), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(),
        QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    /**
     * Destroys the creator.
     */
    virtual ~Creator ();

public slots:

    /**
     * Creates the configured object.
     */
    QObject* create ();

protected:

    /** The meta-object to instantiate. */
    const QMetaObject* _metaObject;

    /** The constructor-specified constructor arguments. */
    QPair<int, void*> _args[10];
};

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
 * Packages up a translation key with optional arguments.
 */
class TranslationKey
{
public:

    /**
     * Creates a new translation key.
     */
    TranslationKey (
        const char* context, const char* sourceText, const char* disambiguation = 0, int n = -1);

    /**
     * Creates an empty, invalid translation key.
     */
    TranslationKey ();

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (const QString& a1);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (const QString& a1, const QString& a2);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (const QString& a1, const QString& a2, const QString& a3);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4,
        const QString& a5);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4,
        const QString& a5, const QString& a6);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4,
        const QString& a5, const QString& a6, const QString& a7);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4,
        const QString& a5, const QString& a6, const QString& a7, const QString& a8);

    /**
     * Populates the key's arguments.
     */
    TranslationKey& arg (
        const QString& a1, const QString& a2, const QString& a3, const QString& a4,
        const QString& a5, const QString& a6, const QString& a7, const QString& a8,
        const QString& a9);

    /**
     * Translates using the supplied translator.
     */
    QString translate (const QTranslator* translator) const;

protected:

    /** The translation context. */
    QByteArray _context;

    /** The source text for translation. */
    QByteArray _sourceText;

    /** The disambiguation string, if any. */
    QByteArray _disambiguation;

    /** The number, or -1 for none. */
    int _n;

    /** Arguments to insert into the translated string. */
    QVector<QString> _args;
};

/**
 * Provides a means of throttling operations that should not occur too frequently (i.e., more than
 * N times per time interval T).
 */
class Throttle
{
public:

    /**
     * Creates a new throttle.
     *
     * @param count the maximum number of operations.
     * @param interval the interval in ms over which the maximum may occur.
     */
    Throttle (int count, quint64 interval);

    /**
     * Attempts an operation.
     *
     * @return true if the operation may proceed; false to throttle the operation because there
     * have been too many, too quickly.
     */
    bool attemptOp ();

protected:

    /** The timestamp buckets. */
    QVector<quint64> _buckets;

    /** The interval over which we track. */
    quint64 _interval;

    /** The current index in the bucket vector. */
    int _bucketIdx;
};

/**
 * Hash function for points.
 */
inline uint qHash (const QPoint& point) { return point.x()*31 + point.y(); }

/**
 * Generates a random token of the specified length.
 */
QByteArray generateToken (int length);

/**
 * Returns the current time in milliseconds since the epoch.
 */
quint64 currentTimeMillis ();

/**
 * Returns a random index corresponding to the given probabilities (which must sum up to one).
 */
int randomIndex (const double* probs);

#endif // GENERAL
