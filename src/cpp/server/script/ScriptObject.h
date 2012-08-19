//
// $Id$

#ifndef SCRIPT_OBJECT
#define SCRIPT_OBJECT

#include <QSharedPointer>

#include "script/Script.h"

/**
 * Base class for script objects.
 */
class ScriptObject
{
public:

    /** The object types. */
    enum Type { SentinelType, StringType, SymbolType, ListType };

    /**
     * Destroys the object.
     */
    virtual ~ScriptObject ();

    /**
     * Returns the type of the object.
     */
    virtual Type type () const = 0;

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const = 0;
};

typedef QSharedPointer<ScriptObject> ScriptObjectPointer;

/**
 * Base class for parsed data.
 */
class Datum : public ScriptObject
{
public:

    /**
     * Creates a new datum.
     */
    Datum (const ScriptPosition& position);

    /**
     * Returns a reference to the datum's position.
     */
    const ScriptPosition& position () const { return _position; }

protected:

    /** The datum's position in the script. */
    ScriptPosition _position;
};

/**
 * A datum used only in parsing that indicates the end of a list.
 */
class Sentinel : public Datum
{
public:

    /**
     * Creates a new sentinel.
     */
    Sentinel (const ScriptPosition& position);

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SentinelType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "SENTINEL"; }
};

/**
 * A datum representing a literal string.
 */
class String : public Datum
{
public:

    /**
     * Creates a new string.
     */
    String (const QString& contents, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the string's contents.
     */
    const QString& contents () const { return _contents; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return StringType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "\"" + _contents + "\""; }

protected:

    /** The string contents. */
    QString _contents;
};

/**
 * A datum representing a symbol.
 */
class Symbol : public Datum
{
public:

    /**
     * Creates a new symbol.
     */
    Symbol (const QString& name, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the symbol name.
     */
    const QString& name () const { return _name; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SymbolType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return _name; }

protected:

    /** The symbol name. */
    QString _name;
};

/**
 * A datum containing a list of other data.
 */
class List : public Datum
{
public:

    /**
     * Creates a new list.
     */
    List (const QList<ScriptObjectPointer>& contents,
        const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the list contents.
     */
    const QList<ScriptObjectPointer>& contents () const { return _contents; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ListType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The contents of the list. */
    const QList<ScriptObjectPointer> _contents;
};

#endif // SCRIPT_OBJECT
