//
// $Id$

#ifndef SCRIPT
#define SCRIPT

#include <QString>

/**
 * Contains a position within a script.
 */
class ScriptPosition
{
public:

    /**
     * Creates a new position.
     */
    ScriptPosition (const QString& expr = QString(), const QString& source = QString(),
        int idx = 0, int line = 0, int lineIdx = 0);

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
    ScriptError (const QString& message, const ScriptPosition& position);

    /**
     * Returns a string representation of the error.
     */
    QString toString (bool compact = false) const;

protected:

    /** A message explaining the error. */
    QString _message;

    /** The location of the error. */
    ScriptPosition _position;
};

#endif // SCRIPT
