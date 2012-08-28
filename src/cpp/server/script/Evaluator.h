//
// $Id$

#ifndef EVALUATOR
#define EVALUATOR

#include <QHash>
#include <QPair>

#include "script/ScriptObject.h"

/**
 * A scope used for compilation.
 */
class Scope
{
public:

    /**
     * Creates a new scope.
     */
    Scope (Scope* parent = 0);

    /**
     * Sets the scope's arguments.
     */
    void setArguments (const QStringList& firstArgs, const QString& restArg);

    /**
     * Attempts to resolve a symbol name, returning a pointer to an Argument or Member if
     * successful.
     */
    ScriptObjectPointer resolve (const QString& name);

    /**
     * Defines a member in this scope with the supplied name.
     */
    ScriptObjectPointer define (const QString& name);

    /**
     * Returns the number of members in this scope.
     */
    int memberCount () const { return _memberCount; }

    /**
     * Returns a reference to the scope's list of constants.
     */
    const QList<ScriptObjectPointer>& constants () const { return _constants; }

    /**
     * Adds a constant and returns its index.
     */
    int addConstant (ScriptObjectPointer value);

    /**
     * Returns a reference to the initialization bytecode.
     */
    QByteArray& initBytecode () { return _initBytecode; }

protected:

    /** The parent scope, if any. */
    Scope* _parent;

    /** Maps variable names to their Argument/Member objects. */
    QHash<QString, ScriptObjectPointer> _variables;

    /** The number of members defined in this scope. */
    int _memberCount;

    /** The list of constants. */
    QList<ScriptObjectPointer> _constants;

    /** The initialization bytecode. */
    QByteArray _initBytecode;
};

/**
 * Allows evaluating scripts.
 */
class Evaluator
{
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
     * Compiles the specified parsed expression in the given scope to the supplied buffer.
     *
     * @param allowDef whether or not to allow a definition.
     */
    void compile (ScriptObjectPointer expr, Scope* scope, bool allowDef, QByteArray& out);

    /**
     * Compiles the body of a lambda/function definition and optionally returns the name of
     * the defined member.
     *
     * @param define if true, we're defining a member in the current scope; return the Member.
     */
    ScriptObjectPointer compileLambda (List* list, Scope* scope, QByteArray& out,
        bool define = false);

    /** The source of the input. */
    QString _source;

    /** The top-level scope. */
    Scope _scope;

    /** The evaluation stack. */
    QStack<ScriptObjectPointer> _stack;

    /** The index of the current procedure on the stack. */
    int _procedureIdx;

    /** The index of the first argument on the stack. */
    int _argumentIdx;

    /** The index of the current instruction in the procedure definition. */
    int _instructionIdx;

    /** The current operand count. */
    int _operandCount;
};

#endif // EVALUATOR
