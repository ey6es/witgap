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

Quote::Quote (const ScriptObjectPointer& datum) :
    _datum(datum)
{
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

SetArgument::SetArgument (int index, const ScriptObjectPointer& expr) :
    Argument(index),
    _expr(expr)
{
}

QString SetArgument::toString () const
{
    return "{set-argument " + QString::number(_index) + " " + _expr->toString() + "}";
}

SetMember::SetMember (int scope, int index, const ScriptObjectPointer& expr) :
    Member(scope, index),
    _expr(expr)
{
}

QString SetMember::toString () const
{
    return "{set-member " + QString::number(_scope) + " " +
        QString::number(_index) + " " + _expr->toString() + "}";
}

ProcedureCall::ProcedureCall (
        const ScriptObjectPointer& procedureExpr,
        const QList<ScriptObjectPointer>& argumentExprs) :
    _procedureExpr(procedureExpr),
    _argumentExprs(argumentExprs)
{
}

QString ProcedureCall::toString () const
{
    QString string("(" + _procedureExpr->toString());
    foreach (const ScriptObjectPointer& argumentExpr, _argumentExprs) {
        string.append(' ');
        string.append(argumentExpr->toString());
    }
    string.append(')');
    return string;
}
