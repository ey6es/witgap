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
    enum Type { SentinelType, BooleanType, StringType, SymbolType, ListType, UnspecifiedType,
        ProcedureType, QuoteType, ArgumentType, MemberType, SetArgumentType, SetMemberType,
        ProcedureCallType, LambdaType, LambdaProcedureType };

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
    virtual QString toString () const { return "{sentinel}"; }
};

/**
 * A datum representing a boolean value.
 */
class Boolean : public Datum
{
public:

    /**
     * Creates a new boolean.
     */
    Boolean (bool value, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns the boolean's value.
     */
    bool value () const { return _value; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return BooleanType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return _value ? "#t" : "#f"; }

protected:

    /** The boolean's value. */
    bool _value;
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

/**
 * An unspecified object.
 */
class Unspecified : public ScriptObject
{
public:

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return UnspecifiedType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "{unspecified}"; }
};

/**
 * A procedure object.
 */
class Procedure : public ScriptObject
{
public:

    /**
     * Calls the procedure and returns the result.
     */
    virtual ScriptObjectPointer call (const QList<ScriptObjectPointer>& args) = 0;

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ProcedureType; }
};

/**
 * Represents a post-bind quoted datum.
 */
class Quote : public ScriptObject
{
public:

    /**
     * Creates a new quoted datum.
     */
    Quote (const ScriptObjectPointer& datum);

    /**
     * Returns a reference to the quoted datum.
     */
    const ScriptObjectPointer& datum () const { return _datum; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return QuoteType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "(quote " + _datum->toString() + ")"; }

protected:

    /** The quoted datum. */
    ScriptObjectPointer _datum;
};

/**
 * Represents a post-bind argument reference.
 */
class Argument : public ScriptObject
{
public:

    /**
     * Creates a new argument reference.
     */
    Argument (int index);

    /**
     * Returns the index of the argument.
     */
    int index () const { return _index; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ArgumentType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "{argument " + QString::number(_index) + "}"; }

protected:

    /** The index of the argument. */
    int _index;
};

/**
 * Represents a post-bind member reference.
 */
class Member : public ScriptObject
{
public:

    /**
     * Creates a new member reference.
     */
    Member (int scope, int index);

    /**
     * Returns the scope number (zero is current, one is parent, etc.)
     */
    int scope () const { return _scope; }

    /**
     * Returns the index of the member.
     */
    int index () const { return _index; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return MemberType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The scope number (zero is current, one is parent, etc.) */
    int _scope;

    /** The index of the member. */
    int _index;
};

/**
 * Represents a post-bind argument set command.
 */
class SetArgument : public Argument
{
public:

    /**
     * Creates a new argument setter.
     */
    SetArgument (int index, const ScriptObjectPointer& expr);

    /**
     * Returns a reference to the bound value expression.
     */
    const ScriptObjectPointer& expr () const { return _expr; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SetArgumentType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The bound value expression. */
    ScriptObjectPointer _expr;
};

/**
 * Represents a post-bind member set command.
 */
class SetMember : public Member
{
public:

    /**
     * Creates a new member setter.
     */
    SetMember (int scope, int index, const ScriptObjectPointer& expr);

    /**
     * Returns a reference to the bound value expression.
     */
    const ScriptObjectPointer& expr () const { return _expr; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SetMemberType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The bound value expression. */
    ScriptObjectPointer _expr;
};

/**
 * Represents a post-bind procedure call command.
 */
class ProcedureCall : public ScriptObject
{
public:

    /**
     * Creates a new procedure call.
     */
    ProcedureCall (const ScriptObjectPointer& procedureExpr,
        const QList<ScriptObjectPointer>& argumentExprs);

    /**
     * Returns the bound expression that should evaluate to the procedure.
     */
    const ScriptObjectPointer& procedureExpr () const { return _procedureExpr; }

    /**
     * Returns a reference to the bound expressions that evaluate to the arguments to pass to the
     * procedure.
     */
    const QList<ScriptObjectPointer>& argumentExprs () const { return _argumentExprs; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ProcedureCallType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The bound expression that should evaluate to the procedure. */
    ScriptObjectPointer _procedureExpr;

    /** The bound expressions that evaluate to the arguments to pass to the procedure. */
    QList<ScriptObjectPointer> _argumentExprs;
};

/**
 * Represents a post-bind lambda expression (i.e., a function definition).
 */
class Lambda : public ScriptObject
{
public:

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return LambdaType; }

protected:

    /** The number of scalar arguments expected by the function. */
    int _scalarArgumentCount;

    /** If true, put the rest of the arguments in a list. */
    bool _listArgument;

    /** The bound initial expressions for each member. */
    QList<ScriptObjectPointer> _memberExprs;

    /** The bound function body expressions. */
    QList<ScriptObjectPointer> _bodyExprs;
};

/**
 * Represents the result of a lambda expression (i.e., a function instance).
 */
class LambdaProcedure : public Procedure
{
public:

    /**
     * Calls the procedure and returns the result.
     */
    virtual ScriptObjectPointer call (const QList<ScriptObjectPointer>& args);

protected:

    /** The current member values. */
    QList<ScriptObjectPointer> _memberExprs;
};

#endif // SCRIPT_OBJECT
