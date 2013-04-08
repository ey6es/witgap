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
        PairType, ListType, VectorType, ByteVectorType, UnspecifiedType, VariableType, LambdaType,
        LambdaProcedureType, InvocationType, NativeProcedureType, CaptureProcedureType,
        EscapeProcedureType, SyntaxRulesType, IdentifierSyntaxType };

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

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);
};

/** A reference-counted pointer to a script object. */
typedef QSharedPointer<ScriptObject> ScriptObjectPointer;

/** A weak pointer to a script object. */
typedef QWeakPointer<ScriptObject> WeakScriptObjectPointer;

/**
 * Compares two script objects according to their types.
 */
bool equivalent (const ScriptObjectPointer& p1, const ScriptObjectPointer& p2);

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
     * Returns a reference to the shared instance with the specified value.
     */
    static const ScriptObjectPointer& instance (bool value);

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
     * Returns an instance with the specified value.  Values from -128 to +127 are represented as
     * shared instances.
     */
    static ScriptObjectPointer instance (int value);

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
     * Returns an instance with the specified name.  Some symbols are represented as shared
     * instances.
     */
    static ScriptObjectPointer instance (const QString& name);

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
 * A datum containing two other data.
 */
class Pair : public Datum
{
public:

    /**
     * Creates a new pair.
     */
    Pair (const ScriptObjectPointer& car, const ScriptObjectPointer& cdr,
        const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the value of the car field.
     */
    const ScriptObjectPointer& car () const { return _car; }

    /**
     * Returns a reference to the value of the cdr field.
     */
    const ScriptObjectPointer& cdr () const { return _cdr; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return PairType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The value of the car field. */
    ScriptObjectPointer _car;

    /** The value of the cdr field. */
    ScriptObjectPointer _cdr;

    /** The mark color. */
    int _color;
};

/** A list of script pointers. */
typedef QList<ScriptObjectPointer> ScriptObjectPointerList;

/**
 * A datum containing a list of other data.
 */
class List : public Datum
{
public:

    /**
     * Returns an instance with the specified contents.  The empty list is represented as a shared
     * instance.
     */
    static ScriptObjectPointer instance (const ScriptObjectPointerList& contents);

    /**
     * Returns an instance containing the single specified element.
     */
    static ScriptObjectPointer instance (const ScriptObjectPointer& element);

    /**
     * Creates a new list.
     */
    List (const ScriptObjectPointerList& contents,
        const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the list contents.
     */
    const ScriptObjectPointerList& contents () const { return _contents; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ListType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The contents of the list. */
    ScriptObjectPointerList _contents;

    /** The mark color. */
    int _color;
};

/**
 * A datum containing a vector of other data.
 */
class Vector : public Datum
{
public:

    /**
     * Returns an instance with the specified contents.  The empty vector is represented as a shared
     * instance.
     */
    static ScriptObjectPointer instance (const ScriptObjectPointerList& contents);

    /**
     * Returns an instance containing the single specified element.
     */
    static ScriptObjectPointer instance (const ScriptObjectPointer& element);

    /**
     * Creates a new vector.
     */
    Vector (const ScriptObjectPointerList& contents,
        const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the vector contents.
     */
    const ScriptObjectPointerList& contents () const { return _contents; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return VectorType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The contents of the vector. */
    ScriptObjectPointerList _contents;

    /** The mark color. */
    int _color;
};

/**
 * A datum containing a vector of bytes.
 */
class ByteVector : public Datum
{
public:

    /**
     * Creates a new byte vector.
     */
    ByteVector (const QByteArray& contents, const ScriptPosition& position = ScriptPosition());

    /**
     * Returns a reference to the vector contents.
     */
    const QByteArray& contents () const { return _contents; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return ByteVectorType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The contents of the vector. */
    QByteArray _contents;
};

/**
 * An unspecified object.
 */
class Unspecified : public ScriptObject
{
public:

    /**
     * Returns a reference to the shared instance.
     */
    static const ScriptObjectPointer& instance ();

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
 * Represents a post-bind variable reference.
 */
class Variable : public ScriptObject
{
public:

    /**
     * Creates a new variable reference.
     */
    Variable (int scope, int index);

    /**
     * Returns the scope number (zero is current, one is parent, etc.)
     */
    int scope () const { return _scope; }

    /**
     * Returns the index of the variable.
     */
    int index () const { return _index; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return VariableType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const;

protected:

    /** The scope number (zero is current, one is parent, etc.) */
    int _scope;

    /** The index of the variable. */
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
    Lambda (int scalarArgumentCount, bool listArgument, int variableCount,
        const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode);

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
     * Returns the total number of variables in the function.
     */
    int variableCount () const { return _variableCount; }

    /**
     * Returns a reference to the constant at the specified index.
     */
    const ScriptObjectPointer& constant (int idx) const { return _constants.at(idx); }

    /**
     * Returns a reference to the constants.
     */
    ScriptObjectPointerList& constants () { return _constants; }

    /**
     * Returns a reference to the bytecode.
     */
    Bytecode& bytecode () { return _bytecode; }

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

    /** The total number of variables. */
    int _variableCount;

    /** The function constants. */
    ScriptObjectPointerList _constants;

    /** The function bytecode. */
    Bytecode _bytecode;
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
     * Returns a reference to the parent invocation.
     */
    ScriptObjectPointer& parent () { return _parent; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return LambdaProcedureType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#lambda_proc_" + QString::number((qulonglong)this, 16); }

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The definition of the procedure. */
    ScriptObjectPointer _lambda;

    /** The procedure's parent invocation. */
    ScriptObjectPointer _parent;

    /** The mark color. */
    int _color;
};

/**
 * Represents the invocation of a lambda procedure.
 */
class Invocation : public ScriptObject
{
public:

    /**
     * Creates a new invocation.
     */
    Invocation (const ScriptObjectPointer& procedure = ScriptObjectPointer(),
        const Registers& registers = Registers());

    /**
     * Returns a reference to the lambda procedure.
     */
    const ScriptObjectPointer& procedure () const { return _procedure; }

    /**
     * Returns a reference to the registers.
     */
    const Registers& registers () const { return _registers; }

    /**
     * Returns a reference to the variable with the specified scope and index.
     */
    const ScriptObjectPointer& variable (int scope, int idx) const;

    /**
     * Returns a reference to the variable list.
     */
    QVector<ScriptObjectPointer>& variables () { return _variables; }

    /**
     * Sets the value of the variable at the specified scope and index.
     */
    void setVariable (int scope, int idx, const ScriptObjectPointer& value);

    /**
     * Appends a local variable.
     */
    void appendVariable (const ScriptObjectPointer& value) { _variables.append(value); }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return InvocationType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#invocation_" + QString::number((qulonglong)this, 16); }

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The lambda procedure being invoked. */
    ScriptObjectPointer _procedure;

    /** The current variable values. */
    QVector<ScriptObjectPointer> _variables;

    /** The stored register values. */
    Registers _registers;

    /** The mark color. */
    int _color;
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
    virtual QString toString () const {
        return "#native_proc_" + QString::number((qulonglong)this, 16); }

protected:

    /** The function to call. */
    Function _function;
};

/**
 * Class for procedures that capture the dynamic environment.
 */
class CaptureProcedure : public ScriptObject
{
public:

    /**
     * Creates a new capture procedure.
     */
    CaptureProcedure ();

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return CaptureProcedureType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#capture_proc_" + QString::number((qulonglong)this, 16); }
};

/**
 * Class for escape procedures, which restore a dynamic environment.
 */
class EscapeProcedure : public ScriptObject
{
public:

    /**
     * Creates a new escape procedure.
     */
    EscapeProcedure (const QStack<ScriptObjectPointer>& stack, const Registers& registers);

    /**
     * Returns a reference to the stack to restore.
     */
    const QStack<ScriptObjectPointer>& stack () const { return _stack; }

    /**
     * Returns a reference to the registers to restore.
     */
    const Registers& registers () const { return _registers; }

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return EscapeProcedureType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#escape_proc_" + QString::number((qulonglong)this, 16); }

    /**
     * Marks the object with the specified color, recursively marking any objects to which
     * references are held.
     */
    virtual void mark (int color);

    /**
     * Checks whether the object is marked with the specified color.  If not, all references
     * are cleared and false is returned.
     */
    virtual bool sweep (int color);

protected:

    /** The stack to restore. */
    QStack<ScriptObjectPointer> _stack;

    /** The registers to restore. */
    Registers _registers;

    /** The mark color. */
    int _color;
};

class Pattern;
class Scope;
class Template;

/** A pointer to a pattern. */
typedef QSharedPointer<Pattern> PatternPointer;

/** A pointer to a template. */
typedef QSharedPointer<Template> TemplatePointer;

/**
 * Contains the information necessary to apply a syntax rule.
 */
class PatternTemplate
{
public:

    /**
     * Creates a new pattern template.
     */
    PatternTemplate (
        int variableCount, const PatternPointer& pattern, const TemplatePointer& templ);

    /**
     * Creates an empty, invalid pattern template.
     */
    PatternTemplate ();

    /**
     * Attempts to match the provided form to our pattern and generate the corresponding
     * template, returning a null pointer if a match couldn't be made.
     */
    ScriptObjectPointer maybeTransform (ScriptObjectPointer form, Scope* scope) const;

protected:

    /** The number of variables in the rule. */
    int _variableCount;

    /** The pattern to match. */
    PatternPointer _pattern;

    /** The template to generate. */
    TemplatePointer _template;
};

/**
 * A macro transformer that contains a set of rules to expand forms.
 */
class SyntaxRules : public ScriptObject
{
public:

    /**
     * Creates a new syntax rules object.
     */
    SyntaxRules (const QVector<PatternTemplate>& patternTemplates);

    /**
     * Attempts to match the provided form to our pattern and generate the corresponding
     * template, returning a null pointer if a match couldn't be made.
     */
    ScriptObjectPointer maybeTransform (ScriptObjectPointer form, Scope* scope) const;

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return SyntaxRulesType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#syntax_rules_" + QString::number((qulonglong)this, 16); }

protected:

    /** The list of pattern/template pairs. */
    QVector<PatternTemplate> _patternTemplates;
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
    IdentifierSyntax (const TemplatePointer& templ, const PatternTemplate& setPatternTemplate);

    /**
     * Generates the output for an identifier occurrence.
     */
    ScriptObjectPointer generate () const;

    /**
     * Attempts to match the provided form to our pattern and generate the corresponding
     * template, returning a null pointer if a match couldn't be made.
     */
    ScriptObjectPointer maybeTransform (ScriptObjectPointer form, Scope* scope) const;

    /**
     * Returns the type of the object.
     */
    virtual Type type () const { return IdentifierSyntaxType; }

    /**
     * Returns a string representation of the object.
     */
    virtual QString toString () const {
        return "#ident_syntax_" + QString::number((qulonglong)this, 16); }

protected:

    /** The template to use when referencing the identifier. */
    TemplatePointer _template;

    /** The (optional) template to use when setting the identifier. */
    PatternTemplate _setPatternTemplate;
};

#endif // SCRIPT_OBJECT
