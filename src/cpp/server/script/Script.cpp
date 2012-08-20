//
// $Id$

#include "script/Script.h"

ScriptPosition::ScriptPosition (
        const QString& expr, const QString& source, int idx, int line, int lineIdx) :
    _expr(expr),
    _source(source),
    _idx(idx),
    _line(line),
    _lineIdx(lineIdx)
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

ScriptError::ScriptError (const QString& message, const ScriptPosition& position) :
    _message(message),
    _position(position)
{
}

QString ScriptError::toString (bool compact) const
{
    if (compact) {
        return _position.toString(true) + ": " + _message;
    }
    return _position.toString(false) + _message;
}
