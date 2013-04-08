//
// $Id$

#include "script/Lexer.h"

Lexer::Lexer (const QString& expr, const QString& source) :
    _expr(expr),
    _source(source),
    _idx(0),
    _line(0),
    _lineIdx(0)
{
}

int Lexer::nextLexeme ()
{
    for (int nn = _expr.length(); _idx < nn; _idx++) {
        QChar ch = _expr.at(_idx);
        if (ch.isSpace()) {
            if (ch == '\n') {
                _line++;
                _lineIdx = _idx + 1;
            }
            continue;
        }
        if (ch == ';') { // comment; ignore everything until the next line
            for (_idx++; _idx < nn; _idx++) {
                QChar ch = _expr.at(_idx);
                if (ch == '\n') {
                    _line++;
                    _lineIdx = _idx + 1;
                    break;
                }
            }
            continue;
        }
        _position = pos();
        if (ch == '(' || ch == ')' || ch == '\'' || ch == '`') {
            _idx++;
            return ch.unicode();
        }
        if (ch == ',') {
            _idx++;
            if (_idx < nn && _expr.at(_idx) == '@') {
                _idx++;
                return UnquoteSplicing;
            }
            return ',';
        }
        if (ch == '\"') {
            _string = "";
            for (_idx++; _idx < nn; _idx++) {
                QChar ch = _expr.at(_idx);
                if (ch == '\\') {
                    if (++_idx == nn) {
                        break; // means the literal is unclosed
                    }
                    ch = _expr.at(_idx);
                    if (ch.isSpace()) { // continuation
                        for (; _idx < nn; _idx++) {
                            ch = _expr.at(_idx);
                            if (ch == '\n') {
                                _line++;
                                _lineIdx = _idx + 1;
                                break;
                            }
                        }
                        for (_idx++; _idx < nn; _idx++) {
                            ch = _expr.at(_idx);
                            if (!ch.isSpace() || ch == '\n') {
                                _idx--;
                                break;
                            }
                        }
                        continue;
                    }
                    switch (ch.unicode()) {
                        case '\"':
                        case '\\':
                            break;

                        case 'n':
                            ch = '\n';
                            break;

                        default:
                            throw ScriptError("Unrecognized escape.", pos());
                    }
                } else if (ch == '\"') {
                    _idx++;
                    return String;
                }
                _string.append(ch);
            }
            throw ScriptError("Unclosed string literal.", _position);
        }
        if (ch == '#') {
            if (_idx + 1 < nn) {
                switch (_expr.at(_idx + 1).unicode()) {
                    case 't':
                    case 'T':
                        _boolValue = true;
                        _idx += 2;
                        return Boolean;

                    case 'f':
                    case 'F':
                        _boolValue = false;
                        _idx += 2;
                        return Boolean;

                    case '(':
                        _idx += 2;
                        return Vector;

                    case 'v':
                        if (_idx + 4 < nn && _expr.mid(_idx + 2, 3) == "u8(") {
                            _idx += 5;
                            return ByteVector;
                        }
                        break;

                    case '\'':
                        _idx += 2;
                        return Syntax;

                    case '`':
                        _idx += 2;
                        return Quasisyntax;

                    case ',':
                        if (_idx + 2 < nn && _expr.at(_idx + 2) == '@') {
                            _idx += 3;
                            return UnsyntaxSplicing;
                        }
                        _idx += 2;
                        return Unsyntax;
                }
            }
        } else if (ch == '+' || ch == '-') {
            if (_idx + 1 < nn) {
                QChar nch = _expr.at(_idx + 1);
                if (nch.isDigit() || nch == '.') {
                    return readNumber();
                }
            }
        } else if (ch == '.') {
            if (_idx + 1 < nn) {
                QChar nch = _expr.at(_idx + 1);
                if (nch.isDigit()) {
                    return readNumber();
                }
            }
        } else if (ch.isDigit()) {
            return readNumber();
        }

        // assume it to be an identifier; search for first non-identifier character or end
        _string = "";
        for (; _idx < nn; _idx++) {
            QChar ch = _expr.at(_idx);
            if (ch.isSpace() || ch == ';' || ch == '(' || ch == ')') {
                break;
            }
            _string.append(ch);
        }
        return Identifier;
    }
    return NoLexeme;
}

Lexer::LexemeType Lexer::readNumber ()
{
    // read until we reach a non-number character, noting if we see a decimal
    QString nstr;
    bool decimal = false;

    for (int nn = _expr.length(); _idx < nn; _idx++) {
        QChar ch = _expr.at(_idx);
        if (ch == '.') {
            nstr.append(ch);
            decimal = true;
            continue;
        }
        if (!(ch.isLetter() || ch.isDigit() || ch == '+' || ch == '-')) {
            break;
        }
        nstr.append(ch);
    }
    bool valid;
    if (decimal) {
        _floatValue = nstr.toFloat(&valid);
        if (!valid) {
            throw ScriptError("Invalid float literal.", _position);
        }
        return Float;

    } else {
        _intValue = nstr.toInt(&valid, 0); // allow 0x### for hex, 0### for octal
        if (!valid) {
            throw ScriptError("Invalid integer literal.", _position);
        }
        return Integer;
    }
}
