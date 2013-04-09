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

/**
 * Checks whether the specified character is a delimiter.
 */
static bool isDelimiter (QChar ch)
{
    return ch.isSpace() || ch == ';' || ch == '#' || ch == '\"' ||
        ch == '(' || ch == ')' || ch == '[' || ch == ']';
}

/**
 * Creates the named character map.
 */
static QHash<QString, QChar> createNamedChars ()
{
    QHash<QString, QChar> hash;
    hash.insert("nul", 0x0);
    hash.insert("alarm", 0x07);
    hash.insert("backspace", 0x08);
    hash.insert("tab", 0x09);
    hash.insert("linefeed", 0x0A);
    hash.insert("newline", 0x0A);
    hash.insert("vtab", 0x0B);
    hash.insert("page", 0x0C);
    hash.insert("return", 0x0D);
    hash.insert("esc", 0x1B);
    hash.insert("space", 0x20);
    hash.insert("delete", 0x7F);
    return hash;
}

/**
 * Returns a reference to the named character map.
 */
static const QHash<QString, QChar>& namedChars ()
{
    static QHash<QString, QChar> namedChars = createNamedChars();
    return namedChars;
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
        if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '\'' || ch == '`') {
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

                        case 'a':
                            ch = '\a';
                            break;

                        case 'b':
                            ch = '\b';
                            break;

                        case 't':
                            ch = '\t';
                            break;

                        case 'n':
                            ch = '\n';
                            break;

                        case 'v':
                            ch = '\v';
                            break;

                        case 'f':
                            ch = '\f';
                            break;

                        case 'r':
                            ch = '\r';
                            break;

                        case 'x': {
                            QString string;
                            for (_idx++; _idx < nn; _idx++) {
                                QChar ch = _expr.at(_idx);
                                if (ch == ';') {
                                    break;
                                }
                                string.append(ch);
                            }
                            bool ok;
                            ch = string.toInt(&ok, 16);
                            if (!ok) {
                                throw ScriptError("Invalid character code.", pos());
                            }
                            break;
                        }
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

                    case '\\':
                        if (_idx + 2 < nn) {
                            QString string(_expr.at(_idx + 2));
                            for (_idx += 3; _idx < nn; _idx++) {
                                QChar ch = _expr.at(_idx);
                                if (isDelimiter(ch)) {
                                    break;
                                }
                                string.append(ch);
                            }
                            if (string.size() == 1) {
                                _charValue = string.at(0);
                                return Char;
                            }
                            QHash<QString, QChar>::const_iterator it = namedChars().find(string);
                            if (it != namedChars().constEnd()) {
                                _charValue = it.value();
                                return Char;
                            }
                            if (string.at(0) != 'x') {
                                throw ScriptError("Unknown character.", _position);
                            }
                            bool ok;
                            _charValue = string.remove(0, 1).toInt(&ok, 16);
                            if (!ok) {
                                throw ScriptError("Invalid character value.", _position);
                            }
                            return Char;
                        }
                        break;

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

                    case '!':
                        if (_idx + 5 < nn && _expr.mid(_idx + 2, 4) == "r6rs") {
                            _idx += 6;
                            continue;
                        }
                        break;

                    case '|': {
                        int depth = 1;
                        for (_idx += 2; _idx < nn; _idx++) {
                            QChar ch = _expr.at(_idx);
                            switch (ch.unicode()) {
                                case '\n':
                                    _line++;
                                    _lineIdx = _idx + 1;
                                    break;

                                case '#':
                                    if (_idx + 1 < nn && _expr.at(_idx + 1) == '|') {
                                        _idx++;
                                        depth++;
                                    }
                                    break;

                                case '|':
                                    if (_idx + 1 < nn && _expr.at(_idx + 1) == '#') {
                                        _idx++;
                                        if (--depth == 0) {
                                            goto outerContinue;
                                        }
                                    }
                                    break;
                            }
                        }
                        break;
                    }
                    case ';':
                        _idx += 2;
                        return Comment;
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
            if (isDelimiter(ch)) {
                break;
            }
            _string.append(ch);
        }
        return Identifier;

        // allow nested loops to continue
        outerContinue: ;
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
