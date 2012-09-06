//
// $Id$

#ifndef SCRIPT_OBJECT
#define SCRIPT_OBJECT

#include <QHash>
#include <QSharedPointer>
#include <QStack>

#include "script/Script.h"

class Evaluator;

/**
 * Base class for script objects.
 */
class ScriptObject
{
public:

    /** The object types. */
    enum Type { SentinelType, BooleanType, IntegerType, FloatType, StringType, SymbolType,
        ListType, UnspecifiedType, ArgumentType, MemberType, LambdaType, LambdaProcedureType,
        NativeProcedureType, ReturnType, SyntaxRulesType, IdentifierSyntaxType };

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
 * A datum representing an integer.
 */
class Integer : public Datum
{
public:

    /**
     * Creates a new integer.
     */
    Integer (int value, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns the integer's value.
     */
    int value () const { return _value; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return IntegerType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return QString::number(_value); }

protected:

    /** The integer's value. */
    int _value;
};

/**
 * A datum representing a floating point number.
 */
class Float : public Datum
{
public:

    /**
     * Creates a new float.
     */
    Float (float value, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns the float's value.
     */
    float value () const { return _value; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return FloatType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return QString::number(_value); }

protected:

    /** The float's value. */
    float _value;
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
 * Represents a post-bind lambda expression (i.e., a function definition).
 */
class Lambda : public ScriptObject
{
public:

    /**
     * Creates a new lambda expression.
     */
    Lambda (int scalarArgumentCount, bool listArgument, int memberCount,
        const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode, int bodyIdx);

    /**
     * Creates a new empty lambda expression.
     */
    Lambda ();

    /**
     * Returns the number of scalar arguments expected by the function.
     */
    int scalarArgumentCount () const { return _scalarArgumentCount; }

    /**
     * Returns whether or not to put the rest of the arguments in a list.
     */
    bool listArgument () const { return _listArgument; }

    /**
     * Returns the number of members in the function.
     */
    int memberCount () const { return _memberCount; }

    /**
     * Sets the constants and bytecode.
     */
    void setConstantsAndBytecode (
        const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode);

    /**
     * Clears out all constants and bytecode.
     */
    void clearConstantsAndBytecode ();

    /**
     * Returns a reference to the constant at the specified index.
     */
    const ScriptObjectPointer& constant (int idx) const { return _constants.at(idx); }

    /**
     * Returns a reference to the bytecode.
     */
    const Bytecode& bytecode () const { return _bytecode; }

    /**
     * Returns a pointer to the initializer bytecode.
     */
    const uchar* initializer () const { return _bytecode.data(); }

    /**
     * Returns a pointer to the body bytecode.
     */
    const uchar* body () const { return _bytecode.data() + _bodyIdx; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return LambdaType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The number of scalar arguments expected by the function. */
    int _scalarArgumentCount;

    /** If true, put the rest of the arguments in a list. */
    bool _listArgument;

    /** The number of members. */
    int _memberCount;

    /** The function constants. */
    QList<ScriptObjectPointer> _constants;

    /** The function bytecode. */
    Bytecode _bytecode;

    /** The index within the bytecode of the function body. */
    int _bodyIdx;
};

/**
 * Represents the result of a lambda expression (i.e., a function instance).
 */
class LambdaProcedure : public ScriptObject
{
public:

    /**
     * Creates a new lambda procedure.
     */
    LambdaProcedure (const ScriptObjectPointer& lambda = ScriptObjectPointer(),
        const ScriptObjectPointer& parent = ScriptObjectPointer());

    /**
     * Returns a reference to the procedure's definition.
     */
    const ScriptObjectPointer& lambda () const { return _lambda; }

    /**
     * Returns a reference to the member with the specified scope and index.
     */
    const ScriptObjectPointer& member (int scope, int idx) const;

    /**
     * Sets the value of the member at the specified scope and index.
     */
    void setMember (int scope, int idx, const ScriptObjectPointer& value);

    /**
     * Appends a local member.
     */
    void appendMember (const ScriptObjectPointer& value) { _members.append(value); }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return LambdaProcedureType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "#lambda_proc_" + QString::number((int)this, 16); }

protected:

    /** The definition of the procedure. */
    ScriptObjectPointer _lambda;

    /** The procedure's parent scope. */
    ScriptObjectPointer _parent;

    /** The current member values. */
    QVector<ScriptObjectPointer> _members;
};

/**
 * Class for native procedures.
 */
class NativeProcedure : public ScriptObject
{
public:

    /** The function type. */
    typedef ScriptObjectPointer (*Function)(Evaluator* eval, int argc, ScriptObjectPointer* argv);

    /**
     * Creates a new native procedure.
     */
    NativeProcedure (Function function);
    
    /**
     * Returns the native function.
     */
    Function function () const { return _function; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return NativeProcedureType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "#native_proc_" + QString::number((int)this, 16); }

protected:

    /** The function to call. */
    Function _function;
};

/**
 * Contains the information necessary to return from a procedure call.
 */
class Return : public ScriptObject
{
public:

    /**
     * Creates a new return.
     */
    Return (const Registers& registers);

    /**
     * Returns a reference to the register values.
     */
    const Registers& registers () const { return _registers; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ReturnType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The stored register values. */
    Registers _registers;
};

class Pattern;
class Template;

/** A pointer to a pattern. */
typedef QSharedPointer<Pattern> PatternPointer;

/** A pointer to a template. */
typedef QSharedPointer<Template> TemplatePointer;

/** A pattern/template pair. */
typedef QPair<PatternPointer, TemplatePointer> PatternTemplatePair;

/**
 * A macro transformer that contains a set of rules to expand forms.
 */
class SyntaxRules : public ScriptObject
{
public:

    /**
     * Creates a new syntax rules object.
     */
    SyntaxRules (const QVector<PatternTemplatePair>& patternTemplatePairs);

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SyntaxRulesType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "#syntax_rules_" + QString::number((int)this, 16); }
    
protected:

    /** The list of pattern/template pairs. */
    QVector<PatternTemplatePair> _patternTemplatePairs;
};

/**
 * A macro transformer that contains templates to expand identifiers.
 */
class IdentifierSyntax : public ScriptObject
{
public:

    /**
     * Creates a new identifier syntax object.
     */
    IdentifierSyntax (const TemplatePointer& templ, const PatternTemplatePair& setPatternTemplate);

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return IdentifierSyntaxType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const { return "#ident_syntax_" + QString::number((int)this, 16); }

protected:
    
    /** The template to use when referencing the identifier. */
    TemplatePointer _template;
    
    /** The (optional) template to use when setting the identifier. */
    PatternTemplatePair _setPatternTemplate;
};

#endif // SCRIPT_OBJECT
