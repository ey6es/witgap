//
// $Id$

#ifndef SCRIPT_CONTEXT
#define SCRIPT_CONTEXT

#include <QVariant>

/**
 * A context in which scripts may be executed.
 */
class ScriptContext
{
public:

    /**
     * Parses the provided string as an expression and returns the result.  Throws ScriptError
     * if parsing fails.
     */
    static QVariant parse (const QString& expr, const QString& source = QString());

    /**
     * Evaluates the provided string as an expression and returns the result.
     */
    QVariant evaluate (const QString& expr) { return evaluate(parse(expr)); }

    /**
     * Evaluates the provided parsed expression and returns the result.
     */
    QVariant evaluate (const QVariant& expr);
};

/**
 * Contains a position within a script and allows tokenization.
 */
class ScriptPosition
{
public:

    /**
     * Creates a new position for tokenization.
     */
    ScriptPosition (const QString& expr, const QString& source);

    /**
     * Returns the next token in the expression, or an invalid variant if there are no more tokens.
     */
    QVariant nextToken ();

    /**
     * Returns a string representation of the position.
     */
    QString toString (bool compact = false) const;

protected:

    /** The expression in string form. */
    QString _expr;

    /** The source of the expression. */
    QString _source;

    /** The index within the string. */
    int _idx;

    /** The line number. */
    int _line;

    /** The index at which the line starts. */
    int _lineIdx;
};

/**
 * Contains information on an error encountered in script evaluation.
 */
class ScriptError
{
public:

    /**
     * Creates a new script error.
     */
    ScriptError (const QString& message, const ScriptPosition& pos);

    /**
     * Returns a string representation of the error.
     */
    QString toString (bool compact = false) const;

protected:

    /** A message explaining the error. */
    QString _message;

    /** The location of the error. */
    ScriptPosition _pos;
};

#endif // SCRIPT_CONTEXT
