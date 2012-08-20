//
// $Id$

#include <QStringList>

#include "script/Evaluator.h"
#include "script/Parser.h"

BindScope::BindScope (BindScope* parent) :
    _parent(parent)
{
}

void BindScope::setArguments (const QStringList& firstArgs, const QString& restArg)
{
    int idx = 0;
    foreach (const QString& arg, firstArgs) {
        _arguments.insert(arg, ScriptObjectPointer(new Argument(idx++)));
    }
    if (!restArg.isEmpty()) {
        _arguments.insert(restArg, ScriptObjectPointer(new Argument(idx++)));
    }
}

ScriptObjectPointer BindScope::resolve (const QString& name)
{
    // first check the local arguments
    ScriptObjectPointer argument = _arguments.value(name);
    if (!argument.isNull()) {
        return argument;
    }

    // then the local members
    ScriptObjectPointer member = _members.value(name).first;
    if (!member.isNull()) {
        return member;
    }

    // try the parent, if any
    if (_parent != 0) {
        ScriptObjectPointer pbinding = _parent->resolve(name);
        if (!pbinding.isNull()) {
            switch (pbinding->type()) {
                case ScriptObject::ArgumentType:
                    // copy parent argument to local member on initialization
                    return define(name, pbinding);

                case ScriptObject::MemberType: {
                    Member* member = static_cast<Member*>(pbinding.data());
                    return ScriptObjectPointer(new Member(member->scope() + 1, member->index()));
                }
            }
        }
    }

    // no luck
    return ScriptObjectPointer();
}

ScriptObjectPointer BindScope::define (const QString& name, ScriptObjectPointer expr)
{
    ScriptObjectPointer member(new Member(0, _members.size()));
    _members.insert(name, ScriptObjectPointerPair(member, expr));
    return member;
}

Scope::Scope (const Scope* parent) :
    _parent(parent)
{
}

void Scope::set (const QString& name, ScriptObjectPointer value)
{
    _objects.insert(name, value);
}

ScriptObjectPointer Scope::resolve (const QString& name) const
{
    ScriptObjectPointer value = _objects.value(name);
    return (!value.isNull() || _parent == 0) ? value : _parent->resolve(name);
}

Evaluator::Evaluator (const QString& source) :
    _source(source)
{
}

ScriptObjectPointer Evaluator::evaluate (const QString& expr)
{
    Parser parser(expr, _source);
    ScriptObjectPointer datum;
    ScriptObjectPointer result;
    while (!(datum = parser.parse()).isNull()) {
        ScriptObjectPointer bound = bind(datum, &_bindScope, true);
        if (bound->type() == ScriptObject::SymbolType) {
            // base level definition; initialize immediately
            Symbol* symbol = static_cast<Symbol*>(bound.data());

        } else {
            result = evaluate(bound, &_scope);
        }
    }
    return result;
}

ScriptObjectPointer Evaluator::bind (ScriptObjectPointer expr, BindScope* scope, bool allowDef)
{
    switch (expr->type()) {
        case ScriptObject::BooleanType:
        case ScriptObject::StringType:
            return expr;

        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            ScriptObjectPointer binding = scope->resolve(symbol->name());
            if (binding.isNull()) {
                throw ScriptError("Unresolved symbol.", symbol->position());
            }
            return binding;
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const QList<ScriptObjectPointer>& contents = list->contents();
            if (contents.isEmpty()) {
                throw ScriptError("Invalid expression.", list->position());
            }
            ScriptObjectPointer car = contents.at(0);
            if (car->type() == ScriptObject::SymbolType) {
                Symbol* symbol = static_cast<Symbol*>(car.data());
                QString name = symbol->name();
                if (name == "define") {
                    if (!allowDef) {
                        throw ScriptError("Definitions not allowed here.", list->position());
                    }
                    if (contents.size() < 2) {
                        throw ScriptError("Invalid definition.", list->position());
                    }
                    ScriptObjectPointer cadr = contents.at(1);
                    switch (cadr->type()) {
                        case ScriptObject::SymbolType: {
                            Symbol* variable = static_cast<Symbol*>(cadr.data());
                            if (contents.size() == 2) {
                                scope->define(variable->name(),
                                    ScriptObjectPointer(new Unspecified()));

                            } else if (contents.size() == 3) {
                                scope->define(variable->name(),
                                    bind(contents.at(2), scope, false));

                            } else {
                                throw ScriptError("Invalid definition.", list->position());
                            }
                            return cadr;
                        }
                        case ScriptObject::ListType: {
                            QString variable;
                            ScriptObjectPointer lambda = bindLambda(list, scope, &variable);
                            if (variable.isEmpty()) {
                                throw ScriptError("Invalid definition.", list->position());
                            }
                            scope->define(variable, lambda);
                        }
                        default:
                            throw ScriptError("Invalid definition.", list->position());
                    }
                } else if (name == "lambda") {
                    return bindLambda(list, scope);

                } else if (name == "set!") {
                    if (contents.size() != 3) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    ScriptObjectPointer variable = contents.at(1);
                    if (variable->type() != ScriptObject::SymbolType) {
                        Datum* datum = static_cast<Datum*>(variable.data());
                        throw ScriptError("Not a variable.", datum->position());
                    }
                    Symbol* var = static_cast<Symbol*>(variable.data());
                    ScriptObjectPointer binding = scope->resolve(var->name());
                    if (binding.isNull()) {
                        throw ScriptError("Unresolved symbol.", var->position());
                    }
                    ScriptObjectPointer value = bind(contents.at(2), scope, false);
                    switch (binding->type()) {
                        case ScriptObject::ArgumentType: {
                            Argument* argument = static_cast<Argument*>(binding.data());
                            return ScriptObjectPointer(new SetArgument(argument->index(), value));
                        }
                        case ScriptObject::MemberType: {
                            Member* member = static_cast<Member*>(binding.data());
                            return ScriptObjectPointer(new SetMember(
                                member->scope(), member->index(), value));
                        }
                    }
                } else if (name == "quote") {
                    if (contents.size() != 2) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    return ScriptObjectPointer(new Quote(contents.at(1)));
                }
            }
            ScriptObjectPointer procedureExpr = bind(car, scope, false);
            QList<ScriptObjectPointer> argumentExprs;
            for (int ii = 1, nn = contents.size(); ii < nn; ii++) {
                argumentExprs.append(bind(contents.at(ii), scope, false));
            }
            return ScriptObjectPointer(new ProcedureCall(procedureExpr, argumentExprs));
        }
    }
}

ScriptObjectPointer Evaluator::bindLambda (List* list, BindScope* scope, QString* firstArg)
{
    const QList<ScriptObjectPointer>& contents = list->contents();
    if (contents.size() < 3) {
        throw ScriptError("Invalid lambda.", list->position());
    }
    ScriptObjectPointer args = contents.at(1);
    QStringList firstArgs;
    QString restArg;
    switch (args->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(args.data());
            restArg = symbol->name();
            break;
        }
        case ScriptObject::ListType: {
            List* alist = static_cast<List*>(args.data());
            bool rest = false;
            foreach (ScriptObjectPointer arg, alist->contents()) {
                if (arg->type() != ScriptObject::SymbolType) {
                    Datum* datum = static_cast<Datum*>(arg.data());
                    throw ScriptError("Invalid argument.", datum->position());
                }
                Symbol* var = static_cast<Symbol*>(arg.data());
                if (firstArg != 0 && firstArg->isEmpty()) {
                    *firstArg = var->name();
                    continue;
                }
                if (rest) {
                    if (var->name() == "." || !restArg.isEmpty()) {
                        throw ScriptError("Invalid argument.", var->position());
                    }
                    restArg = var->name();
                    continue;
                }
                if (var->name() == ".") {
                    rest = true;
                } else {
                    firstArgs.append(var->name());
                }
            }
            break;
        }
        default:
            Datum* datum = static_cast<Datum*>(args.data());
            throw ScriptError("Invalid formals.", datum->position());
    }
    BindScope subscope(scope);
    bool exprs = false;
    QList<ScriptObjectPointer> memberExprs;
    QList<ScriptObjectPointer> bodyExprs;
    for (int ii = 2, nn = contents.size(); ii < nn; ii++) {
        ScriptObjectPointer bound = bind(contents.at(ii), &subscope, true);
        if (bound.isNull()) {

        }

    }
    return ScriptObjectPointer();
}

ScriptObjectPointer Evaluator::evaluate (ScriptObjectPointer expr, Scope* scope)
{
    switch (expr->type()) {
        case ScriptObject::BooleanType:
        case ScriptObject::StringType:
        case ScriptObject::UnspecifiedType:
            return expr;

        case ScriptObject::QuoteType: {
            Quote* quote = static_cast<Quote*>(expr.data());
            return quote->datum();
        }
        case ScriptObject::ArgumentType: {
            Argument* argument = static_cast<Argument*>(expr.data());
        }
        case ScriptObject::MemberType: {
            Member* member = static_cast<Member*>(expr.data());
        }
        case ScriptObject::SetArgumentType: {
            SetArgument* setter = static_cast<SetArgument*>(expr.data());
            evaluate(setter->expr(), scope);
        }
        case ScriptObject::SetMemberType: {
            SetMember* setter = static_cast<SetMember*>(expr.data());
            evaluate(setter->expr(), scope);
        }
        case ScriptObject::ProcedureCallType: {
            ProcedureCall* call = static_cast<ProcedureCall*>(expr.data());

        }
        case ScriptObject::LambdaType: {
            Lambda* lambda = static_cast<Lambda*>(expr.data());

        }
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            ScriptObjectPointer value = scope->resolve(symbol->name());
            if (value.isNull()) {
                throw ScriptError("Unresolved symbol.", symbol->position());
            }
            return value;
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const QList<ScriptObjectPointer>& contents = list->contents();
            if (contents.isEmpty()) {
                throw ScriptError("Invalid expression.", list->position());
            }
            ScriptObjectPointer car = contents.at(0);
            if (car->type() == ScriptObject::SymbolType) {
                Symbol* symbol = static_cast<Symbol*>(car.data());
                QString name = symbol->name();
                if (name == "define") {

                    return ScriptObjectPointer();

                } else if (name == "lambda") {
                    if (contents.size() < 3) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    ScriptObjectPointer args = contents.at(1);
                    QList<QString> firstArgs;
                    QString restArg;
                    switch (args->type()) {
                        case ScriptObject::SymbolType: {
                            Symbol* symbol = static_cast<Symbol*>(args.data());
                            restArg = symbol->name();
                            break;
                        }
                        case ScriptObject::ListType: {
                            List* alist = static_cast<List*>(args.data());
                            foreach (ScriptObjectPointer arg, alist->contents()) {

                            }
                            break;
                        }
                        default:
                            Datum* datum = static_cast<Datum*>(args.data());
                            throw ScriptError("Invalid formals.", datum->position());
                    }

                } else if (name == "set!") {
                    if (contents.size() != 3) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    ScriptObjectPointer variable = contents.at(1);
                    if (variable->type() != ScriptObject::SymbolType) {
                        Datum* datum = static_cast<Datum*>(variable.data());
                        throw ScriptError("Not a variable.", datum->position());
                    }
                    Symbol* var = static_cast<Symbol*>(variable.data());

                    return ScriptObjectPointer(new Unspecified());

                } else if (name == "quote") {
                    if (contents.size() != 2) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    return contents.at(1);
                }
            }
            ScriptObjectPointer fn = evaluate(car, scope);
            if (fn.isNull() || fn->type() != ScriptObject::ProcedureType) {
                Datum* datum = static_cast<Datum*>(car.data());
                throw ScriptError("Doesn't evaluate to procedure.", datum->position());
            }
            QList<ScriptObjectPointer> args;
            for (int ii = 1, nn = contents.size(); ii < nn; ii++) {
                ScriptObjectPointer element = contents.at(ii);
                ScriptObjectPointer result = evaluate(element, scope);
                if (result.isNull()) {
                    Datum* datum = static_cast<Datum*>(element.data());
                    throw ScriptError("Doesn't evaluate to value.", datum->position());
                }
                args.append(result);
            }
            Procedure* procedure = static_cast<Procedure*>(fn.data());
            return procedure->call(args);
        }
    }
}
