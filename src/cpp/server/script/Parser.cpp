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
        case Lexer::Vector: {
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
            return ScriptObjectPointer(new Vector(contents, position));
        }
        case Lexer::ByteVector: {
            ScriptPosition position = _lexer.position();
            QByteArray contents;
            ScriptObjectPointer datum;
            while (!(datum = parseDatum()).isNull() &&
                    datum->type() != ScriptObject::SentinelType) {
                ScriptPosition dpos = static_cast<Datum*>(datum.data())->position();
                if (datum->type() != ScriptObject::IntegerType) {
                    throw ScriptError("Invalid byte value.", dpos);
                }
                int value = static_cast<Integer*>(datum.data())->value();
                if (value < 0 || value > 255) {
                    throw ScriptError("Invalid byte value.", dpos);
                }
                contents.append(value);
            }
            if (datum.isNull()) {
                throw ScriptError("Missing end parenthesis.", position);
            }
            return ScriptObjectPointer(new ByteVector(contents, position));
        }
        case ')':
            return ScriptObjectPointer(new Sentinel(_lexer.position()));

        case '\'':
            return parseAbbreviation("quote");

        case '`':
            return parseAbbreviation("quasiquote");

        case ',':
            return parseAbbreviation("unquote");

        case Lexer::UnquoteSplicing:
            return parseAbbreviation("unquote-splicing");

        case Lexer::Syntax:
            return parseAbbreviation("syntax");

        case Lexer::Quasisyntax:
            return parseAbbreviation("quasisyntax");

        case Lexer::Unsyntax:
            return parseAbbreviation("unsyntax");

        case Lexer::UnsyntaxSplicing:
            return parseAbbreviation("unsyntax-splicing");

        case Lexer::Comment:
            parseDatum();
            return parseDatum();

        case Lexer::NoLexeme:
            return ScriptObjectPointer();

        case Lexer::Identifier:
            return ScriptObjectPointer(new Symbol(_lexer.string(), _lexer.position()));

        case Lexer::Boolean:
            return ScriptObjectPointer(new Boolean(_lexer.boolValue(), _lexer.position()));

        case Lexer::Integer:
            return ScriptObjectPointer(new Integer(_lexer.intValue(), _lexer.position()));

        case Lexer::Float:
            return ScriptObjectPointer(new Float(_lexer.floatValue(), _lexer.position()));

        case Lexer::String:
            return ScriptObjectPointer(new String(_lexer.string(), _lexer.position()));

        default:
            throw ScriptError("Unrecognized lexeme.", _lexer.position());
    }
}

ScriptObjectPointer Parser::parseAbbreviation (const QString& symbol)
{
    ScriptPosition position = _lexer.position();
    QList<ScriptObjectPointer> contents;
    contents.append(ScriptObjectPointer(new Symbol(symbol, position)));
    contents.append(parseDatum());
    return ScriptObjectPointer(new List(contents, position));
}
