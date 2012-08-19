//
// $Id$

#ifndef PARSER
#define PARSER

#include "script/Lexer.h"
#include "script/ScriptObject.h"

/**
 * Parses expressions into data.
 */
class Parser
{
public:

    /**
     * Creates a parser for the supplied expression.
     */
    Parser (const QString& expr, const QString& source = QString());

    /**
     * Parses and returns the next datum, or an invalid pointer if there are no more.  Throws
     * ScriptError if a parse error occurs.
     */
    ScriptObjectPointer parse ();

protected:

    /**
     * Parses and returns the next datum, or an invalid pointer if there are no more.  Throws
     * ScriptError if a parse error occurs.
     */
    ScriptObjectPointer parseDatum ();

    /** The lexer that breaks the expression up into lexemes. */
    Lexer _lexer;
};

#endif // PARSER
