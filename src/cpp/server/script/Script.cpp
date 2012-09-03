//
// $Id$

#include <QtEndian>

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

void Bytecode::append (BytecodeOp op, const ScriptPosition& pos)
{
    _positions.insert(_data.size(), pos);
    append(op);
}

void Bytecode::append (BytecodeOp op, int p1)
{
    int size = _data.size();
    _data.resize(size + 5);
    uchar* dst = (uchar*)(_data.data() + size);
    *dst = op;
    qToBigEndian<qint32>(p1, dst + 1);
}

void Bytecode::append (BytecodeOp op, int p1, int p2)
{
    int size = _data.size();
    _data.resize(size + 9);
    uchar* dst = (uchar*)(_data.data() + size);
    *dst = op;
    qToBigEndian<qint32>(p1, dst + 1);
    qToBigEndian<qint32>(p2, dst + 5);
}

void Bytecode::append (const Bytecode& bytecode)
{
    int offset = _data.size();
    _data.append(bytecode._data);
    for (QHash<int, ScriptPosition>::const_iterator it = bytecode._positions.constBegin(),
            end = bytecode._positions.constEnd(); it != end; it++) {
        _positions.insert(it.key() + offset, it.value());
    }
}
