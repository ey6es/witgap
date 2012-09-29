//
// $Id$

#ifndef EVALUATOR
#define EVALUATOR

#include <QHash>
#include <QStringList>

#include "script/ScriptObject.h"

/**
 * A scope used for compilation.
 */
class Scope
{
public:

    /**
     * Creates a new scope.
     *
     * @param withValues if true, create an associated invocation to hold member values.
     * @param syntactic if true, this is a purely syntactic scope; it will not have an associated
     * invocation.
     */
    Scope (Scope* parent = 0, bool withValues = false, bool syntactic = false);

    /**
     * Returns a pointer to the scope's first non-syntactic ancestor.
     */
    Scope* nonSyntacticAncestor ();

    /**
     * Returns a reference to the internal invocation.
     */
    const ScriptObjectPointer& invocation () const { return _invocation; }

    /**
     * Attempts to resolve a symbol name, returning a pointer to a Variable, etc., if
     * successful.
     */
    ScriptObjectPointer resolve (const QString& name);

    /**
     * Returns the number of variables defined in the scope.
     */
    int variableCount () const { return _variableCount; }

    /**
     * Defines a native function in the scope.
     */
    ScriptObjectPointer addVariable (const QString& name, NativeProcedure::Function function);

    /**
     * Defines a variable in this scope with the supplied name and optional initialization
     * expression/value.
     */
    ScriptObjectPointer addVariable (const QString& name,
        const ScriptObjectPointer& initExpr = ScriptObjectPointer(),
        const ScriptObjectPointer& value = ScriptObjectPointer());

    /**
     * Defines an arbitrary object in the scope.
     */
    void define (const QString& name, const ScriptObjectPointer& binding);

    /**
     * Adds a constant and returns its index.
     */
    int addConstant (ScriptObjectPointer value);

    /**
     * Returns a reference to the list of constants.
     */
    ScriptObjectPointerList& constants () { return _constants; }

    /**
     * Returns a reference to the deferred definitions.
     */
    ScriptObjectPointerList& deferred () { return _deferred; }

protected:

    /** The parent scope, if any. */
    Scope* _parent;

    /** Whether or not this is a purely syntactic scope. */
    bool _syntactic;

    /** Maps symbol names to their Variable/SyntaxRules/IdentifierSyntax objects. */
    QHash<QString, ScriptObjectPointer> _bindings;

    /** The number of variables defined in the scope. */
    int _variableCount;

    /** The list of constants. */
    ScriptObjectPointerList _constants;

    /** The deferred definitions. */
    ScriptObjectPointerList _deferred;
    
    /** An optional invocation associated with the scope, containing variable values. */
    ScriptObjectPointer _invocation;
};

/**
 * Allows evaluating scripts.
 */
class Evaluator : public QObject
{
    Q_OBJECT
    
public:

    /**
     * Creates a new evaluator.
     */
    Evaluator (const QString& source = QString());

    /**
     * Parses and evaluates an expression, returning the result.  Throws ScriptError if an error
     * occurs.
     */
    ScriptObjectPointer evaluate (const QString& expr);

    /**
     * Runs the evaluator for a fixed number of cycles, or until a result is returned.
     *
     * @param maxCycles the maximum number of cycles to execute, or zero for unlimited.
     * @return the result of the computation, or a null pointer if still working.
     */
    ScriptObjectPointer execute (int maxCycles = 0);

protected:

    /**
     * Throws a script error with the supplied message.
     */
    void throwScriptError (const QString& message);

    /** The source of the input. */
    QString _source;
    
    /** The evaluator-level scope. */
    Scope _scope;

    /** The evaluation stack. */
    QStack<ScriptObjectPointer> _stack;

    /** The current register values. */
    Registers _registers;
};

#endif // EVALUATOR
