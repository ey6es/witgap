//
// $Id$

#ifndef LEXER
#define LEXER

#include "script/Script.h"

/**
 * Breaks scripts up into streams of lexemes.
 */
class Lexer
{
public:

    /** The non-character lexeme types. */
    enum LexemeType { NoLexeme, Identifier = 0x10000, String };

    /**
     * Creates a new lexer to analyze the provided expression.
     */
    Lexer (const QString& expr, const QString& source = QString());

    /**
     * Finds the next lexeme in the stream and returns its type (either a single character or a
     * member of LexemeType).  Throws ScriptError if an error is found.
     */
    int nextLexeme ();

    /**
     * Returns a reference to the position of the current lexeme.
     */
    const ScriptPosition& position () const { return _position; }

    /**
     * Returns a reference to the string content of the current lexeme, if applicable.
     */
    const QString& string () const { return _string; }

protected:

    /**
     * Returns the current position as a ScriptPosition.
     */
    ScriptPosition pos () const { return ScriptPosition(_expr, _source, _idx, _line, _lineIdx); }

    /** The expression we're analyzing. */
    QString _expr;

    /** The source of the expression. */
    QString _source;

    /** The index within the string. */
    int _idx;

    /** The current line number. */
    int _line;

    /** The index at which the current line starts. */
    int _lineIdx;

    /** The position of the current lexeme. */
    ScriptPosition _position;

    /** The string content of the current lexeme, if applicable. */
    QString _string;
};

#endif // LEXER
