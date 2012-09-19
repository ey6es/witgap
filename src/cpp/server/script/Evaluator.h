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
     * @param withValues if true, create an associated lambda procedure to hold member values.
     * @param syntactic if true, this is a purely syntactic scope; it will not have an associated
     * lambda.
     */
    Scope (Scope* parent = 0, bool withValues = false, bool syntactic = false);

    /**
     * Returns a reference to the internal lambda procedure.
     */
    const ScriptObjectPointer& lambdaProc () const { return _lambdaProc; }

    /**
     * Sets the scope's arguments.
     */
    void setArguments (const QStringList& firstArgs, const QString& restArg);

    /**
     * Attempts to resolve a symbol name, returning a pointer to an Argument or Member, etc., if
     * successful.
     */
    ScriptObjectPointer resolve (const QString& name);

    /**
     * Defines a native function in the scope.
     */
    ScriptObjectPointer addMember (const QString& name, NativeProcedure::Function function);

    /**
     * Defines a member in this scope with the supplied name and optional value.
     */
    ScriptObjectPointer addMember (const QString& name,
        const ScriptObjectPointer& value = ScriptObjectPointer());

    /**
     * Defines an arbitrary object in the scope.
     */
    void define (const QString& name, const ScriptObjectPointer& value);

    /**
     * Returns the number of members in this scope.
     */
    int memberCount () const { return _memberCount; }

    /**
     * Returns a reference to the scope's list of constants.
     */
    QList<ScriptObjectPointer>& constants () { return _constants; }

    /**
     * Adds a constant and returns its index.
     */
    int addConstant (ScriptObjectPointer value);

    /**
     * Returns a reference to the initialization bytecode.
     */
    Bytecode& initBytecode () { return _initBytecode; }

    /**
     * Returns a reference to the initialization expressions for each member.
     */
    ScriptObjectPointerList& initExprs () { return _initExprs; }

protected:

    /** The parent scope, if any. */
    Scope* _parent;

    /** Whether or not this is a purely syntactic scope. */
    bool _syntactic;

    /** Maps variable names to their Argument/Member/SyntaxRules/IdentifierSyntax objects. */
    QHash<QString, ScriptObjectPointer> _variables;

    /** The number of members defined in this scope. */
    int _memberCount;

    /** The list of constants. */
    QList<ScriptObjectPointer> _constants;

    /** The initialization bytecode. */
    Bytecode _initBytecode;
    
    /** The initialization expressions for each member. */
    ScriptObjectPointerList _initExprs;
    
    /** An optional lambda procedure associated with the scope, containing member values. */
    ScriptObjectPointer _lambdaProc;
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
