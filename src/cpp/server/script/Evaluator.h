//
// $Id$

#ifndef EVALUATOR
#define EVALUATOR

#include <QHash>
#include <QPair>

#include "script/ScriptObject.h"

/**
 * A scope used for binding.
 */
class BindScope
{
public:

    /**
     * Creates a new bind scope.
     */
    BindScope (BindScope* parent = 0);

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
     * Defines a member in this scope with the supplied name and bound initialization expression.
     */
    ScriptObjectPointer define (const QString& name, ScriptObjectPointer expr);

protected:

    /** A pair of pointers to script objects. */
    typedef QPair<ScriptObjectPointer, ScriptObjectPointer> ScriptObjectPointerPair;

    /** The parent scope, if any. */
    BindScope* _parent;

    /** Maps argument names to Argument objects. */
    QHash<QString, ScriptObjectPointer> _arguments;

    /** Maps member names to their indices and initialization expressions. */
    QHash<QString, ScriptObjectPointerPair> _members;
};

/**
 * A scope in which symbol names are mapped to objects.
 */
class Scope
{
public:

    /**
     * Creates a new scope.
     */
    Scope (const Scope* parent = 0);

    /**
     * Assigns a value to a symbol name in this scope.
     */
    void set (const QString& name, ScriptObjectPointer value);

    /**
     * Attempts to resolve a symbol name.
     */
    ScriptObjectPointer resolve (const QString& name) const;

protected:

    /** Maps symbol names to objects. */
    QHash<QString, ScriptObjectPointer> _objects;

    /** The parent scope, if any. */
    const Scope* _parent;
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

protected:

    /**
     * Binds the specified parsed expression in the given scope and returns the result.
     *
     * @param allowDef whether or not to allow a definition.
     */
    ScriptObjectPointer bind (ScriptObjectPointer expr, BindScope* scope, bool allowDef);

    /**
     * Binds and returns the body of a lambda/function definition.
     *
     * @param firstArg if non-zero, place the first arg here instead of processing as usual.
     */
    ScriptObjectPointer bindLambda (List* list, BindScope* scope, QString* firstArg = 0);

    /**
     * Evaluates a parsed expression in the specified scope.
     */
    ScriptObjectPointer evaluate (ScriptObjectPointer expr, Scope* scope);

    /** The source of the input. */
    QString _source;

    /** The top-level bind scope. */
    BindScope _bindScope;

    /** The top-level scope. */
    Scope _scope;
};

#endif // EVALUATOR
