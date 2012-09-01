//
// $Id$

#include "script/ScriptObject.h"

ScriptObject::~ScriptObject ()
{
}

Datum::Datum (const ScriptPosition& position) :
    _position(position)
{
}

Sentinel::Sentinel (const ScriptPosition& position) :
    Datum(position)
{
}

Boolean::Boolean (bool value, const ScriptPosition& position) :
    Datum(position),
    _value(value)
{
}

Integer::Integer (int value, const ScriptPosition& position) :
    Datum(position),
    _value(value)
{
}

Float::Float (float value, const ScriptPosition& position) :
    Datum(position),
    _value(value)
{
}

String::String (const QString& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents)
{
}

Symbol::Symbol (const QString& name, const ScriptPosition& position) :
    Datum(position),
    _name(name)
{
}

List::List (const QList<ScriptObjectPointer>& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents)
{
}

QString List::toString () const
{
    QString string("(");
    foreach (const ScriptObjectPointer& datum, _contents) {
        if (string.length() != 1) {
            string.append(' ');
        }
        string.append(datum->toString());
    }
    string.append(')');
    return string;
}

Argument::Argument (int index) :
    _index(index)
{
}

Member::Member (int scope, int index) :
    _scope(scope),
    _index(index)
{
}

QString Member::toString () const
{
    return "{member " + QString::number(_scope) + " " + QString::number(_index) + "}";
}

Lambda::Lambda (int scalarArgumentCount, bool listArgument, int memberCount,
        const QList<ScriptObjectPointer>& constants, const QByteArray& bytecode, int bodyIdx) :
    _scalarArgumentCount(scalarArgumentCount),
    _listArgument(listArgument),
    _memberCount(memberCount),
    _constants(constants),
    _bytecode(bytecode),
    _bodyIdx(bodyIdx)
{
}

QString Lambda::toString () const
{
    return "{lambda " + QString::number(_scalarArgumentCount) + " " +
        (_listArgument ? "#t" : "#f") + "}";
}

LambdaProcedure::LambdaProcedure (
        const ScriptObjectPointer& lambda, const ScriptObjectPointer& parent) :
    _lambda(lambda),
    _parent(parent)
{
    Lambda* lam = static_cast<Lambda*>(lambda.data());
    _members.resize(lam->memberCount());
}

const ScriptObjectPointer& LambdaProcedure::member (int scope, int idx) const
{
    if (scope == 0) {
        return _members.at(idx);
    }
    LambdaProcedure* parent = static_cast<LambdaProcedure*>(_parent.data());
    return parent->member(scope - 1, idx);
}

void LambdaProcedure::setMember (int scope, int idx, const ScriptObjectPointer& value)
{
    if (scope == 0) {
        _members[idx] = value;
    }
    LambdaProcedure* parent = static_cast<LambdaProcedure*>(_parent.data());
    parent->setMember(scope - 1, idx, value);
}

Return::Return (int procedureIdx, int argumentIdx, const quint8* instruction, int operandCount) :
    _procedureIdx(procedureIdx),
    _argumentIdx(argumentIdx),
    _instruction(instruction),
    _operandCount(operandCount)
{
}

QString Return::toString () const
{
    return "{return " + QString::number(_procedureIdx) + " " +
        QString::number(_argumentIdx) + " " +
        QString::number(_operandCount) + "}";
}
