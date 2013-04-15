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
    QChar bracket = ']';
    switch (_lexer.nextLexeme()) {
        case '(':
            bracket = ')';

        case '[': {
            ScriptPosition position = _lexer.position();
            ScriptObjectPointer datum, firstPair, lastPair;
            ScriptObject::Type type;
            while (!(datum = parseDatum()).isNull() &&
                    (type = datum->type()) != ScriptObject::SentinelType) {
                Datum* dptr = static_cast<Datum*>(datum.data());
                if (type == ScriptObject::SymbolType) {
                    Symbol* symbol = static_cast<Symbol*>(dptr);
                    if (symbol->name() == ".") {
                        datum = parseDatum();
                        if (datum.isNull()) {
                            break;
                        }
                        if (firstPair.isNull() || datum->type() == ScriptObject::SentinelType) {
                            throw ScriptError("Invalid improper list.", position);
                        }
                        Pair* last = static_cast<Pair*>(lastPair.data());
                        last->setCdr(datum);
                        datum = parseDatum();
                        if (!datum.isNull() && datum->type() != ScriptObject::SentinelType) {
                            throw ScriptError("Invalid improper list.", position);
                        }
                        break;
                    }
                }
                ScriptObjectPointer pair = ScriptObjectPointer(
                    new Pair(datum, ScriptObjectPointer(), dptr->position()));

                if (firstPair.isNull()) {
                    firstPair = pair;

                } else {
                    Pair* last = static_cast<Pair*>(lastPair.data());
                    last->setCdr(pair);
                }
                lastPair = pair;
            }
            if (datum.isNull()) {
                throw ScriptError("Missing end parenthesis.", position);
            }
            Sentinel* sentinel = static_cast<Sentinel*>(datum.data());
            if (sentinel->character() != bracket) {
                throw ScriptError("Expected '" + QString(bracket) + "'.", sentinel->position());
            }
            if (firstPair.isNull()) {
                return ScriptObjectPointer(new Null(position));
            }
            Pair* last = static_cast<Pair*>(lastPair.data());
            if (last->cdr().isNull()) {
                last->setCdr(ScriptObjectPointer(new Null(sentinel->position())));
            }
            return firstPair;
        }
        case Lexer::Vector: {
            ScriptPosition position = _lexer.position();
            ScriptObjectPointerVector contents;
            ScriptObjectPointer datum;
            while (!(datum = parseDatum()).isNull() &&
                    datum->type() != ScriptObject::SentinelType) {
                contents.append(datum);
            }
            if (datum.isNull()) {
                throw ScriptError("Missing end parenthesis.", position);
            }
            Sentinel* sentinel = static_cast<Sentinel*>(datum.data());
            if (sentinel->character() != ')') {
                throw ScriptError("Expected ')'.", sentinel->position());
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
            Sentinel* sentinel = static_cast<Sentinel*>(datum.data());
            if (sentinel->character() != ')') {
                throw ScriptError("Expected ')'.", sentinel->position());
            }
            return ScriptObjectPointer(new ByteVector(contents, position));
        }
        case ')':
            return ScriptObjectPointer(new Sentinel(')', _lexer.position()));

        case ']':
            return ScriptObjectPointer(new Sentinel(']', _lexer.position()));

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

        case Lexer::Char:
            return ScriptObjectPointer(new Char(_lexer.charValue(), _lexer.position()));

        case Lexer::String:
            return ScriptObjectPointer(new String(_lexer.string(), _lexer.position()));

        default:
            throw ScriptError("Unrecognized lexeme.", _lexer.position());
    }
}

ScriptObjectPointer Parser::parseAbbreviation (const QString& symbol)
{
    ScriptPosition position = _lexer.position();
    ScriptObjectPointer datum = parseDatum();
    Datum* dptr = static_cast<Datum*>(datum.data());
    return ScriptObjectPointer(new Pair(
        ScriptObjectPointer(new Symbol(symbol, position)),
        ScriptObjectPointer(new Pair(datum, ScriptObjectPointer(
            new Null(dptr->position())), dptr->position())),
        position));
}
