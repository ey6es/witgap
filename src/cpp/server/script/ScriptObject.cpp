//
// $Id$

#include "script/Evaluator.h"
#include "script/MacroTransformer.h"
#include "script/ScriptObject.h"

ScriptObject::~ScriptObject ()
{
}

bool equivalent (const ScriptObjectPointer& p1, const ScriptObjectPointer& p2)
{
    ScriptObject::Type t1 = p1->type();
    if (t1 != p2->type()) {
        return false;
    }
    switch (t1) {
        case ScriptObject::BooleanType: {
            Boolean* b1 = static_cast<Boolean*>(p1.data());
            Boolean* b2 = static_cast<Boolean*>(p2.data());
            return b1->value() == b2->value();
        }
        case ScriptObject::IntegerType: {
            Integer* i1 = static_cast<Integer*>(p1.data());
            Integer* i2 = static_cast<Integer*>(p2.data());
            return i1->value() == i2->value();
        }
        case ScriptObject::FloatType: {
            Float* f1 = static_cast<Float*>(p1.data());
            Float* f2 = static_cast<Float*>(p2.data());
            return f1->value() == f2->value();
        }
        case ScriptObject::StringType: {
            String* s1 = static_cast<String*>(p1.data());
            String* s2 = static_cast<String*>(p2.data());
            return s1->contents() == s2->contents();
        }
        case ScriptObject::SymbolType: {
            Symbol* s1 = static_cast<Symbol*>(p1.data());
            Symbol* s2 = static_cast<Symbol*>(p2.data());
            return s1->name() == s2->name();
        }
        case ScriptObject::ListType: {
            List* l1 = static_cast<List*>(p1.data());
            List* l2 = static_cast<List*>(p2.data());
            const QList<ScriptObjectPointer>& c1 = l1->contents();
            const QList<ScriptObjectPointer>& c2 = l2->contents();
            int size = c1.size();
            if (c2.size() != size) {
                return false;
            }
            for (int ii = 0; ii < size; ii++) {
                if (!equivalent(c1.at(ii), c2.at(ii))) {
                    return false;
                }
            }
            return true;
        }
        case ScriptObject::ArgumentType: {
            Argument* a1 = static_cast<Argument*>(p1.data());
            Argument* a2 = static_cast<Argument*>(p2.data());
            return a1->index() == a2->index();
        }
        case ScriptObject::MemberType: {
            Member* m1 = static_cast<Member*>(p1.data());
            Member* m2 = static_cast<Member*>(p2.data());
            return m1->scope() == m2->scope() && m1->index() == m2->index();
        }
        default:
            return p1.data() == p2.data();
    }
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

List::List (const ScriptObjectPointerList& contents, const ScriptPosition& position) :
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
        const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode, int bodyIdx) :
    _scalarArgumentCount(scalarArgumentCount),
    _listArgument(listArgument),
    _memberCount(memberCount),
    _constants(constants),
    _bytecode(bytecode),
    _bodyIdx(bodyIdx)
{
}

Lambda::Lambda () :
    _scalarArgumentCount(0),
    _listArgument(false),
    _memberCount(0),
    _bodyIdx(0)
{
}

void Lambda::setConstantsAndBytecode (
    const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode)
{
    _constants = constants;
    _bytecode = bytecode;
}

void Lambda::clearConstantsAndBytecode ()
{
    _constants.clear();
    _bytecode.clear();
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
        return;
    }
    LambdaProcedure* parent = static_cast<LambdaProcedure*>(_parent.data());
    parent->setMember(scope - 1, idx, value);
}

NativeProcedure::NativeProcedure (Function function) :
    _function(function)
{
}

Return::Return (const Registers& registers) :
    _registers(registers)
{
}

QString Return::toString () const
{
    return "{return " + QString::number(_registers.procedureIdx) + " " +
        QString::number(_registers.argumentIdx) + " " +
        QString::number(_registers.operandCount) + "}";
}

PatternTemplate::PatternTemplate (
        int variableCount, const PatternPointer& pattern, const TemplatePointer& templ) :
    _variableCount(variableCount),
    _pattern(pattern),
    _template(templ)
{
}

PatternTemplate::PatternTemplate ()
{
}

ScriptObjectPointer PatternTemplate::maybeTransform (ScriptObjectPointer form, Scope* scope) const
{
    if (_pattern.isNull()) {
        return ScriptObjectPointer();
    }
    QVector<ScriptObjectPointer> variables(_variableCount);
    return _pattern->matches(form, scope, variables) ?
        _template->generate(variables) : ScriptObjectPointer();
}

SyntaxRules::SyntaxRules (const QVector<PatternTemplate>& patternTemplates) :
    _patternTemplates(patternTemplates)
{
}

ScriptObjectPointer SyntaxRules::maybeTransform (ScriptObjectPointer form, Scope* scope) const
{
    ScriptObjectPointer result;
    foreach (const PatternTemplate& templ, _patternTemplates) {
        result = templ.maybeTransform(form, scope);
        if (!result.isNull()) {
            break;
        }
    }
    return result;
}

IdentifierSyntax::IdentifierSyntax (
        const TemplatePointer& templ, const PatternTemplate& setPatternTemplate) :
    _template(templ),
    _setPatternTemplate(setPatternTemplate)
{
}

ScriptObjectPointer IdentifierSyntax::generate () const
{
    QVector<ScriptObjectPointer> variables;
    return _template->generate(variables);
}

ScriptObjectPointer IdentifierSyntax::maybeTransform (ScriptObjectPointer form, Scope* scope) const
{
    return _setPatternTemplate.maybeTransform(form, scope);
}
