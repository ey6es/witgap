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
    foreach (ScriptObjectPointer datum, _contents) {
        if (string.length() != 1) {
            string.append(' ');
        }
        string.append(datum->toString());
    }
    string.append(')');
    return string;
}
