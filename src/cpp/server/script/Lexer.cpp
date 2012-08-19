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
        _position = pos();
        if (ch == '(' || ch == ')') {
            _idx++;
            return ch.unicode();
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
        // assume it to be an identifier; search for first non-identifier character or end
        _string = "";
        for (; _idx < nn; _idx++) {
            QChar ch = _expr.at(_idx);
            if (ch.isSpace() || ch == '(' || ch == ')') {
                break;
            }
            _string.append(ch);
        }
        return Identifier;
    }
    return NoLexeme;
}
