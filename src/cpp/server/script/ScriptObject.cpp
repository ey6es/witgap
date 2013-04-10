//
// $Id$

#include "script/Evaluator.h"
#include "script/MacroTransformer.h"
#include "script/ScriptObject.h"

ScriptObject::~ScriptObject ()
{
}

void ScriptObject::mark (int color)
{
    // nothing by default
}

bool ScriptObject::sweep (int color)
{
    return false;
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
        case ScriptObject::CharType: {
            Char* c1 = static_cast<Char*>(p1.data());
            Char* c2 = static_cast<Char*>(p2.data());
            return c1->value() == c2->value();
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
        case ScriptObject::PairType: {
            Pair* r1 = static_cast<Pair*>(p1.data());
            Pair* r2 = static_cast<Pair*>(p2.data());
            return equivalent(r1->car(), r2->car()) && equivalent(r1->cdr(), r2->cdr());
        }
        case ScriptObject::NullType:
            return true;

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
        case ScriptObject::VectorType: {
            Vector* v1 = static_cast<Vector*>(p1.data());
            Vector* v2 = static_cast<Vector*>(p2.data());
            const QList<ScriptObjectPointer>& c1 = v1->contents();
            const QList<ScriptObjectPointer>& c2 = v2->contents();
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
        case ScriptObject::ByteVectorType: {
            ByteVector* v1 = static_cast<ByteVector*>(p1.data());
            ByteVector* v2 = static_cast<ByteVector*>(p2.data());
            return v1->contents() == v2->contents();
        }
        case ScriptObject::VariableType: {
            Variable* v1 = static_cast<Variable*>(p1.data());
            Variable* v2 = static_cast<Variable*>(p2.data());
            return v1->scope() == v2->scope() && v1->index() == v2->index();
        }
        default:
            return p1.data() == p2.data();
    }
}

Datum::Datum (const ScriptPosition& position) :
    _position(position)
{
}

Sentinel::Sentinel (QChar character, const ScriptPosition& position) :
    Datum(position),
    _character(character)
{
}

const ScriptObjectPointer& Boolean::instance (bool value)
{
    static ScriptObjectPointer trueInstance(new Boolean(true));
    static ScriptObjectPointer falseInstance(new Boolean(false));
    return value ? trueInstance : falseInstance;
}

Boolean::Boolean (bool value, const ScriptPosition& position) :
    Datum(position),
    _value(value)
{
}

/**
 * Creates the set of shared Integer instances.
 */
static ScriptObjectPointer* createSharedIntegerInstances ()
{
    ScriptObjectPointer* instances = new ScriptObjectPointer[256];
    for (int ii = 0; ii < 256; ii++) {
        instances[ii] = ScriptObjectPointer(new Integer(-128 + ii));
    }
    return instances;
}

ScriptObjectPointer Integer::instance (int value)
{
    static ScriptObjectPointer* sharedInstances = createSharedIntegerInstances();
    return (value >= -128 && value <= +127) ? sharedInstances[value + 128] :
        ScriptObjectPointer(new Integer(value));
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

/**
 * Creates the set of shared Char instances.
 */
static ScriptObjectPointer* createSharedCharInstances ()
{
    ScriptObjectPointer* instances = new ScriptObjectPointer[128];
    for (int ii = 0; ii < 128; ii++) {
        instances[ii] = ScriptObjectPointer(new Char(ii));
    }
    return instances;
}

ScriptObjectPointer Char::instance (QChar value)
{
    static ScriptObjectPointer* sharedInstances = createSharedCharInstances();
    int unicode = value.unicode();
    return (unicode < 128) ? sharedInstances[unicode] :
        ScriptObjectPointer(new Char(value));
}

Char::Char (QChar value, const ScriptPosition& position) :
    Datum(position),
    _value(value)
{
}

QString Char::toString () const
{
    return "#\\x" + QString::number(_value.unicode(), 16);
}

String::String (const QString& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents)
{
}

/**
 * Creates the map of shared symbol instances.
 */
static QHash<QString, ScriptObjectPointer> createSharedSymbolInstances ()
{
    QHash<QString, ScriptObjectPointer> instances;
    instances.insert("lambda", ScriptObjectPointer(new Symbol("lambda")));
    return instances;
}

ScriptObjectPointer Symbol::instance (const QString& name)
{
    static QHash<QString, ScriptObjectPointer> sharedInstances = createSharedSymbolInstances();
    ScriptObjectPointer value = sharedInstances.value(name);
    return value.isNull() ? ScriptObjectPointer(new Symbol(name)) : value;
}

Symbol::Symbol (const QString& name, const ScriptPosition& position) :
    Datum(position),
    _name(name)
{
}

Pair::Pair (const ScriptObjectPointer& car, const ScriptObjectPointer& cdr,
        const ScriptPosition& position) :
    Datum(position),
    _car(car),
    _cdr(cdr),
    _color(0)
{
}

QString Pair::toString () const
{
    QString string("(");

    for (const Pair* pair = this;; ) {
        string.append(pair->car()->toString());
        switch (pair->cdr()->type()) {
            case PairType:
                string.append(' ');
                pair = static_cast<Pair*>(pair->cdr().data());
                break;

            case NullType:
                goto outerBreak;

            default:
                string.append(" . ");
                string.append(pair->cdr()->toString());
                goto outerBreak;
        }
    }
    outerBreak:

    string.append(')');
    return string;
}

void Pair::mark (int color)
{
    if (_color != color) {
        _color = color;
        _car->mark(color);
        _cdr->mark(color);
    }
}

bool Pair::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _car.clear();
    _cdr.clear();
    return false;
}

const ScriptObjectPointer& Null::instance ()
{
    static ScriptObjectPointer instance(new Null());
    return instance;
}

Null::Null (const ScriptPosition& position) :
    Datum(position)
{
}

ScriptObjectPointer List::instance (const ScriptObjectPointerList& contents)
{
    static ScriptObjectPointer emptyInstance(new List(ScriptObjectPointerList()));
    return contents.isEmpty() ? emptyInstance : ScriptObjectPointer(new List(contents));
}

ScriptObjectPointer List::instance (const ScriptObjectPointer& element)
{
    ScriptObjectPointerList contents;
    contents.append(element);
    return ScriptObjectPointer(new List(contents));
}

List::List (const ScriptObjectPointerList& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents),
    _color(0)
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

void List::mark (int color)
{
    if (_color != color) {
        _color = color;
        foreach (const ScriptObjectPointer& element, _contents) {
            element->mark(color);
        }
    }
}

bool List::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _contents.clear();
    return false;
}

ScriptObjectPointer Vector::instance (const ScriptObjectPointerList& contents)
{
    static ScriptObjectPointer emptyInstance(new Vector(ScriptObjectPointerList()));
    return contents.isEmpty() ? emptyInstance : ScriptObjectPointer(new Vector(contents));
}

ScriptObjectPointer Vector::instance (const ScriptObjectPointer& element)
{
    ScriptObjectPointerList contents;
    contents.append(element);
    return ScriptObjectPointer(new Vector(contents));
}

Vector::Vector (const ScriptObjectPointerList& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents),
    _color(0)
{
}

QString Vector::toString () const
{
    QString string("#(");
    foreach (const ScriptObjectPointer& datum, _contents) {
        if (string.length() != 2) {
            string.append(' ');
        }
        string.append(datum->toString());
    }
    string.append(')');
    return string;
}

void Vector::mark (int color)
{
    if (_color != color) {
        _color = color;
        foreach (const ScriptObjectPointer& element, _contents) {
            element->mark(color);
        }
    }
}

bool Vector::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _contents.clear();
    return false;
}

ByteVector::ByteVector (const QByteArray& contents, const ScriptPosition& position) :
    Datum(position),
    _contents(contents)
{
}

QString ByteVector::toString () const
{
    QString string("#vu8(");
    for (int ii = 0, nn = _contents.size(); ii < nn; ii++) {
        if (ii > 0) {
            string.append(' ');
        }
        string.append(QString::number((unsigned char)_contents.at(ii)));
    }
    string.append(')');
    return string;
}

const ScriptObjectPointer& Unspecified::instance ()
{
    static ScriptObjectPointer instance(new Unspecified());
    return instance;
}

Variable::Variable (int scope, int index) :
    _scope(scope),
    _index(index)
{
}

QString Variable::toString () const
{
    return "{variable " + QString::number(_scope) + " " + QString::number(_index) + "}";
}

Lambda::Lambda (int scalarArgumentCount, bool listArgument, int variableCount,
        const QList<ScriptObjectPointer>& constants, const Bytecode& bytecode) :
    _scalarArgumentCount(scalarArgumentCount),
    _listArgument(listArgument),
    _variableCount(variableCount),
    _constants(constants),
    _bytecode(bytecode)
{
}

Lambda::Lambda () :
    _scalarArgumentCount(0),
    _listArgument(false),
    _variableCount(0)
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
    _parent(parent),
    _color(0)
{
}

void LambdaProcedure::mark (int color)
{
    if (_color != color) {
        _color = color;
        if (!_parent.isNull()) {
            _parent->mark(color);
        }
    }
}

bool LambdaProcedure::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _parent.clear();
    return false;
}

Invocation::Invocation (const ScriptObjectPointer& procedure, const Registers& registers) :
    _procedure(procedure),
    _variables(static_cast<Lambda*>(static_cast<LambdaProcedure*>(
            procedure.data())->lambda().data())->variableCount(),
        Unspecified::instance()),
    _registers(registers),
    _color(0)
{
}

const ScriptObjectPointer& Invocation::variable (int scope, int idx) const
{
    if (scope == 0) {
        return _variables.at(idx);
    }
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(_procedure.data());
    Invocation* parent = static_cast<Invocation*>(proc->parent().data());
    return parent->variable(scope - 1, idx);
}

void Invocation::setVariable (int scope, int idx, const ScriptObjectPointer& value)
{
    if (scope == 0) {
        _variables[idx] = value;
        return;
    }
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(_procedure.data());
    Invocation* parent = static_cast<Invocation*>(proc->parent().data());
    parent->setVariable(scope - 1, idx, value);
}

void Invocation::mark (int color)
{
    if (_color != color) {
        _color = color;
        _procedure->mark(color);
        foreach (const ScriptObjectPointer& element, _variables) {
            element->mark(color);
        }
    }
}

bool Invocation::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _variables.clear();
    return false;
}

NativeProcedure::NativeProcedure (Function function) :
    _function(function)
{
}

CaptureProcedure::CaptureProcedure ()
{
}

EscapeProcedure::EscapeProcedure (
        const QStack<ScriptObjectPointer>& stack, const Registers& registers) :
    _stack(stack),
    _registers(registers),
    _color(0)
{
}

void EscapeProcedure::mark (int color)
{
    if (_color != color) {
        _color = color;
        foreach (const ScriptObjectPointer& element, _stack) {
            element->mark(color);
        }
    }
}

bool EscapeProcedure::sweep (int color)
{
    if (_color == color) {
        return true;
    }
    _stack.clear();
    return false;
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
