//
// $Id$

#include "script/Parser.h"

Parser::Parser (const QString& expr, const QString& source) :
    _lexer(expr, source)
{
}

ScriptObjectPointer Parser::parse ()
{
    ScriptObjectPointer datum = parseDatum();
    if (!datum.isNull() && datum->type() == ScriptObject::SentinelType) {
        Sentinel* sentinel = static_cast<Sentinel*>(datum.data());
        throw ScriptError("Missing start parenthesis.", sentinel->position());
    }
    return datum;
}

ScriptObjectPointer Parser::parseDatum ()
{
    switch (_lexer.nextLexeme()) {
        case '(': {
            ScriptPosition position = _lexer.position();
            QList<ScriptObjectPointer> contents;
            ScriptObjectPointer datum;
            while (!(datum = parseDatum()).isNull() &&
                    datum->type() != ScriptObject::SentinelType) {
                contents.append(datum);
            }
            if (datum.isNull()) {
                throw ScriptError("Missing end parenthesis.", position);
            }
            return ScriptObjectPointer(new List(contents, position));
        }
        case ')':
            return ScriptObjectPointer(new Sentinel(_lexer.position()));

        case Lexer::NoLexeme:
            return ScriptObjectPointer();

        case Lexer::Identifier:
            return ScriptObjectPointer(new Symbol(_lexer.string(), _lexer.position()));

        case Lexer::String:
            return ScriptObjectPointer(new String(_lexer.string(), _lexer.position()));

        default:
            throw ScriptError("Unrecognized lexeme.", _lexer.position());
    }
}
