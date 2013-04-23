//
// $Id$

#ifndef EVALUATOR
#define EVALUATOR

#include <QHash>
#include <QLinkedList>
#include <QStringList>
#include <QTimer>

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
     * Returns the depth of the scope.
     */
    int depth () const { return _depth; }

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
        const ScriptObjectPointer& value = Unspecified::instance());

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
    ScriptObjectPointerVector& constants () { return _constants; }

    /**
     * Returns a reference to the deferred definitions.
     */
    ScriptObjectPointerVector& deferred () { return _deferred; }

protected:

    /** The depth of the scope. */
    int _depth;

    /** The parent scope, if any. */
    Scope* _parent;

    /** Whether or not this is a purely syntactic scope. */
    bool _syntactic;

    /** Maps symbol names to their Variable/SyntaxRules/IdentifierSyntax objects. */
    QHash<QString, ScriptObjectPointer> _bindings;

    /** The number of variables defined in the scope. */
    int _variableCount;

    /** The list of constants. */
    ScriptObjectPointerVector _constants;

    /** The deferred definitions. */
    ScriptObjectPointerVector _deferred;

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
     *
     * @param input the input device, or 0 to use stdin.
     * @param output the output device, or 0 to use stdout.
     * @param error the error device, or 0 to use stderr.
     */
    Evaluator (const QString& source = QString(), QIODevice* input = 0,
        QIODevice* output = 0, QIODevice* error = 0, QObject* parent = 0);

    /**
     * Returns a reference to the evaluator's standard input port.
     */
    const ScriptObjectPointer& standardInputPort () const { return _standardInputPort; }

    /**
     * Returns a reference to the evaluator's standard output port.
     */
    const ScriptObjectPointer& standardOutputPort () const { return _standardOutputPort; }

    /**
     * Returns a reference to the evaluator's standard error port.
     */
    const ScriptObjectPointer& standardErrorPort () const { return _standardErrorPort; }

    /**
     * Returns the maximum number of cycles being executed in each time slice.
     */
    int maxCyclesPerSlice () const { return _maxCyclesPerSlice; }

    /**
     * Parses and evaluates an expression synchronously, returning the result.  Throws ScriptError
     * if an error occurs.
     */
    ScriptObjectPointer evaluateUntilExit (const QString& expr);

    /**
     * Parses and evaluates an expression asynchronously.
     */
    void evaluate (const QString& expr, int maxCyclesPerSlice = 100);

    /**
     * Cancels the current evaluation (if any).
     */
    void cancel ();

    /**
     * Runs the evaluator for a fixed number of cycles, or until a result is returned.
     *
     * @param maxCycles the maximum number of cycles to execute, or -1 for unlimited.
     * @return the result of the computation, or a null pointer if still working.
     */
    ScriptObjectPointer execute (int maxCycles = -1);

    /**
     * Collects garbage.
     */
    void gc ();

    /**
     * Sets the object responsible for waking us.
     */
    void setWaker (QObject* waker) { _waker = waker; }

    /**
     * Creates a list instance containing the elements in (first, first + count].
     */
    ScriptObjectPointer listInstance (const ScriptObjectPointer* first, int count);

    /**
     * Creates a pair instance and adds it to the collectable list.
     */
    ScriptObjectPointer pairInstance (
        const ScriptObjectPointer& car, const ScriptObjectPointer& cdr = ScriptObjectPointer());

    /**
     * Creates a vector instance and adds it to the collectable list.
     */
    ScriptObjectPointer vectorInstance (const ScriptObjectPointerVector& contents);

signals:

    /**
     * Fired when the script is canceled.
     */
    void canceled ();

    /**
     * Fired when the script exits.
     */
    void exited (const ScriptObjectPointer& result);

    /**
     * Fired when the script encounters an error.
     */
    void threwError (const ScriptError& error);

public slots:

    /**
     * If the sender (an IODevice) is ready to read a line, disconnects and wakes the evaluator up
     * with it.
     */
    void maybeReadLine ();

    /**
     * Wakes up the evaluator.
     */
    void wakeUp (const ScriptObjectPointer& returnValue = Unspecified::instance());

protected slots:

    /**
     * Continues the current thread of execution.
     */
    void continueExecuting ();

protected:

    /**
     * Compiles the supplied expression in preparation for evaluation.  Throws ScriptError if an
     * error occurs.
     */
    void compileForEvaluation (const QString& expr);

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

    /** Weak pointers to the collectable objects we've created. */
    QLinkedList<WeakScriptObjectPointer> _collectable;

    /** The last color used for garbage collection. */
    int _lastColor;

    /** The timer used for execution. */
    QTimer _timer;

    /** The maximum number of cycles to execute in each time slice. */
    int _maxCyclesPerSlice;

    /** Whether or not the evaluator is sleeping. */
    bool _sleeping;

    /** The object responsible for waking us, if known. */
    QObject* _waker;

    /** The evaluator's standard input port. */
    ScriptObjectPointer _standardInputPort;

    /** The evaluator's standard output port. */
    ScriptObjectPointer _standardOutputPort;

    /** The evaluator's standard error port. */
    ScriptObjectPointer _standardErrorPort;
};

/**
 * Throws a ScriptError with the supplied message and position if the given object isn't a Pair;
 * otherwise, returns the casted pointer.
 */
Pair* requirePair (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position);

/**
 * Throws a ScriptError with the supplied message and position if the given object isn't a Symbol;
 * otherwise, returns the casted pointer.
 */
Symbol* requireSymbol (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position);

/**
 * Throws a ScriptError with the supplied message and position if the given object isn't a Null;
 * otherwise, returns the casted pointer.
 */
Null* requireNull (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position);

#endif // EVALUATOR
