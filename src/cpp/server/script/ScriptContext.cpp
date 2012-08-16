//
// $Id$

#include <QtDebug>

#include "script/ScriptContext.h"

QVariant ScriptContext::parse (const QString& expr, const QString& source)
{
    ScriptPosition tokenizer(expr, source);
    QVariant token;
    while ((token = tokenizer.nextToken()).isValid()) {
        qDebug() << token;
    }
    return QVariant();
}

QVariant ScriptContext::evaluate (const QVariant& expr)
{
    return QVariant();
}

ScriptPosition::ScriptPosition (const QString& expr, const QString& source) :
    _expr(expr),
    _source(source),
    _idx(0),
    _line(0),
    _lineIdx(0)
{
}

QString ScriptPosition::toString (bool compact) const
{
    bool sourced = !_source.isEmpty();
    bool multiline = _expr.contains('\n');
    QString string;
    if (compact) {
        if (sourced) {
            string += _source + ":";
        }
        if (multiline) {
            string += QString::number(_line + 1) + ":";
        }
        string += QString::number(_idx - _lineIdx + 1);
        return string;
    }
    if (sourced || multiline) {
        if (sourced) {
            string += _source + ", ";
        }
        if (multiline) {
            string += "line " + QString::number(_line + 1) + ", ";
        }
        string += "column " + QString::number(_idx - _lineIdx + 1) + ":\n";
    }
    int eidx = _expr.indexOf('\n', _lineIdx);
    string += _expr.mid(_lineIdx, (eidx == -1 ? _expr.length() : eidx) - _lineIdx) + '\n';
    string += QString(_idx - _lineIdx, ' ') + "^\n";
    return string;
}

QVariant ScriptPosition::nextToken ()
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
        if (ch == '(' || ch == ')') {
            _idx++;
            return QVariant(ch);
        }
        if (ch == '\"') {
            QString string;
            ScriptPosition start = *this;
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
                            throw ScriptError("Unrecognized escape.", *this);
                    }
                } else if (ch == '\"') {
                    _idx++;
                    return QVariant(string);
                }
                string.append(ch);
            }
            throw ScriptError("Unclosed string literal.", start);
        }
        // assume it to be an identifier; search for first non-identifier character or end
        QString identifier;
        for (; _idx < nn; _idx++) {
            QChar ch = _expr.at(_idx);
            if (ch.isSpace() || ch == '(' || ch == ')') {
                break;
            }
            identifier.append(ch);
        }
        return QVariant(identifier);
    }
    return QVariant();
}

ScriptError::ScriptError (const QString& message, const ScriptPosition& pos) :
    _message(message),
    _pos(pos)
{
}

QString ScriptError::toString (bool compact) const
{
    if (compact) {
        return _pos.toString(true) + ": " + _message;
    }
    return _pos.toString(false) + _message;
}
