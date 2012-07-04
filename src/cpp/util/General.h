//
// $Id$

#ifndef GENERAL
#define GENERAL

#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QPoint>
#include <QVariant>
#include <QVector>

class QIODevice;
class QString;
class QTranslator;

/** A hash from string to variant list. */
typedef QHash<QString, QVariantList> QVariantListHash;

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

public slots:

    /**
     * Creates the configured object.
     */
    QObject* create ();

protected:

    /** The meta-object to instantiate. */
    const QMetaObject* _metaObject;

    /** The constructor-specified constructor arguments. */
    QVariant _args[10];
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
 * Generic descriptor for various resources (zones, scenes, etc.)
 */
class ResourceDescriptor
{
public:

    /** The resource id. */
    quint32 id;

    /** The resource name. */
    QString name;

    /** The user id of the resource creator. */
    quint32 creatorId;

    /** The name of the resource creator. */
    QString creatorName;

    /** The time at which the resource was created. */
    QDateTime created;
};

/** A descriptor for the lack of a resource. */
const ResourceDescriptor NoResource = { 0 };

/** A list of resource descriptors. */
typedef QList<ResourceDescriptor> ResourceDescriptorList;

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

/**
 * "Ungets" an integer (in big endian order) from the specified device.
 */
void unget (QIODevice* device, quint32 value);

/**
 * Equivalent to the SIGNAL() macro; returns the signal equivalent of the supplied signature.
 */
QByteArray signal (const char* signature);

#endif // GENERAL
