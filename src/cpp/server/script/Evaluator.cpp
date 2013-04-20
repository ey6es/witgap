//
// $Id$

#include <QtDebug>

#include "script/Evaluator.h"
#include "script/Globals.h"
#include "script/MacroTransformer.h"
#include "script/Parser.h"

Scope::Scope (Scope* parent, bool withValues, bool syntactic) :
    _depth(parent == 0 ? 0 : parent->depth() + (syntactic ? 0 : 1)),
    _parent(parent),
    _syntactic(syntactic),
    _variableCount(0)
{
    if (withValues) {
        ScriptObjectPointer lambda(new Lambda());
        ScriptObjectPointer proc(new LambdaProcedure(lambda,
            parent == 0 ? ScriptObjectPointer() : parent->invocation()));
        _invocation = ScriptObjectPointer(new Invocation(proc));
    }
}

Scope* Scope::nonSyntacticAncestor ()
{
    return _syntactic ? _parent->nonSyntacticAncestor() : this;
}

ScriptObjectPointer Scope::resolve (const QString& name)
{
    // first check the local bindings, then the parent's
    ScriptObjectPointer binding = _bindings.value(name);
    return (!binding.isNull() || _parent == 0) ? binding : _parent->resolve(name);
}

ScriptObjectPointer Scope::addVariable (const QString& name, NativeProcedure::Function function)
{
    return addVariable(name, ScriptObjectPointer(),
        ScriptObjectPointer(new NativeProcedure(function)));
}

ScriptObjectPointer Scope::addVariable (
    const QString& name, const ScriptObjectPointer& initExpr, const ScriptObjectPointer& value)
{
    if (_syntactic) { // syntactic scopes do not contain variables
        return _parent->addVariable(name, initExpr, value);
    }
    int idx = _variableCount++;
    ScriptObjectPointer variable(new Variable(_depth, idx));
    define(name, variable);
    _deferred.append(initExpr);
    if (!_invocation.isNull()) {
        Invocation* invocation = static_cast<Invocation*>(_invocation.data());
        invocation->appendVariable(value);
    }
    return variable;
}

void Scope::define (const QString& name, const ScriptObjectPointer& binding)
{
    _bindings.insert(name, binding);
}

int Scope::addConstant (ScriptObjectPointer value)
{
    if (_syntactic) { // syntactic scopes do not contain constants
        return _parent->addConstant(value);
    }
    _constants.append(value);
    return _constants.size() - 1;
}

Evaluator::Evaluator (const QString& source) :
    _source(source),
    _scope(globalScope(), true),
    _registers(),
    _lastColor(0),
    _sleeping(false)
{
    _stack.push(_scope.invocation());

    connect(&_timer, SIGNAL(timeout()), SLOT(continueExecuting()));
}

/**
 * Compiles the specified parsed expression in the given scope to the supplied buffer.
 *
 * @param top whether or not this is the top level of a body.
 * @param allowDef whether or not to allow definitions.
 * @param tailCall whether or not this is a tail call context.
 */
static void compile (
    ScriptObjectPointer expr, Scope* scope, bool top, bool allowDef, bool tailCall, Bytecode& out);

/**
 * Compiles a syntax scope.
 */
static void compileSyntax (
    Pair* pair, Scope* scope, bool top, bool allowDef, bool tailCall, bool rec, Bytecode& out)
{
    Pair* cdr = requirePair(pair->cdr(), "Invalid syntax scope.", pair->position());
    Scope subscope(scope, false, true);
    Scope* escope = rec ? &subscope : scope;
    for (ScriptObjectPointer bindings = cdr->car();; ) {
        switch (bindings->type()) {
            case ScriptObject::NullType:
                goto outerBreak;

            case ScriptObject::PairType: {
                Pair* pbindings = static_cast<Pair*>(bindings.data());
                Pair* bpair = requirePair(pbindings->car(), "Invalid syntax binding.",
                    pbindings->position());
                Symbol* symbol = requireSymbol(bpair->car(), "Invalid syntax binding.",
                    pbindings->position());
                bpair = requirePair(bpair->cdr(), "Invalid syntax binding.",
                    pbindings->position());
                requireNull(bpair->cdr(), "Invalid syntax binding.", pbindings->position());
                subscope.define(symbol->name(), createMacroTransformer(bpair->car(), escope));
                bindings = pbindings->cdr();
                break;
            }
            default:
                throw ScriptError("Invalid syntax bindings.", pair->position());
        }
    }
    outerBreak:

    for (ScriptObjectPointer rest = cdr->cdr();; ) {
        switch (rest->type()) {
            case ScriptObject::NullType:
                return;

            case ScriptObject::PairType: {
                Pair* prest = static_cast<Pair*>(rest.data());
                compile(prest->car(), &subscope, top, allowDef, tailCall &&
                    prest->cdr()->type() == ScriptObject::NullType, out);
                rest = prest->cdr();
                break;
            }
            default:
                throw ScriptError("Invalid syntax scope.", pair->position());
        }
    }
}

/**
 * Ensures that the deferred state in the scope is processed.
 */
static void compileDeferred (Scope* scope, bool top, Bytecode& out)
{
    if (!top) {
        return;
    }
    scope = scope->nonSyntacticAncestor();
    ScriptObjectPointerVector& deferred = scope->deferred();
    if (deferred.isEmpty()) {
        return;
    }
    for (int ii = 0, nn = deferred.size(); ii < nn; ii++) {
        ScriptObjectPointer expr = deferred.at(ii);
        if (!expr.isNull()) {
            compile(expr, scope, false, false, false, out);
            out.append(SetVariableOp, 0, scope->variableCount() - nn + ii);
        }
    }
    deferred.clear();
}

/**
 * Appends a pop operation if at the top level and not in the tail position.
 */
static void maybeAppendPop (bool top, bool tailCall, Bytecode& out)
{
    if (top && !tailCall) {
        out.append(PopOp);
    }
}

/**
 * Compiles the body of a lambda/function definition.
 */
static void compileLambda (Pair* pair, Scope* scope, Bytecode& out)
{
    Pair* cdr = requirePair(pair->cdr(), "Invalid lambda.", pair->position());
    Pair* cddr = requirePair(cdr->cdr(), "Invalid lambda.", pair->position());
    Scope subscope(scope);
    int firstArgs = 0;
    bool restArg = false;

    ScriptObjectPointer cadr = cdr->car();
    switch (cadr->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(cadr.data());
            subscope.addVariable(symbol->name());
            restArg = true;
            break;
        }
        case ScriptObject::NullType:
            break;

        case ScriptObject::PairType: {
            for (Pair* rest = static_cast<Pair*>(cadr.data());; ) {
                Symbol* symbol = requireSymbol(rest->car(), "Invalid argument.", rest->position());
                subscope.addVariable(symbol->name());
                firstArgs++;

                ScriptObjectPointer rcdr = rest->cdr();
                switch (rcdr->type()) {
                    case ScriptObject::NullType:
                        goto argsBreak;

                    case ScriptObject::PairType:
                        rest = static_cast<Pair*>(rcdr.data());
                        break;

                    case ScriptObject::SymbolType:
                        symbol = static_cast<Symbol*>(rcdr.data());
                        subscope.addVariable(symbol->name());
                        restArg = true;
                        goto argsBreak;

                    default: {
                        Datum* datum = static_cast<Datum*>(rcdr.data());
                        throw ScriptError("Invalid argument.", datum->position());
                    }
                }
            }
            argsBreak:
            break;
        }
        default:
            throw ScriptError("Invalid formals.", pair->position());
    }

    Bytecode bytecode;
    for (Pair* rest = cddr;; ) {
        ScriptObjectPointer rcdr = rest->cdr();
        compile(rest->car(), &subscope, true, true,
            rcdr->type() == ScriptObject::NullType, bytecode);

        switch (rcdr->type()) {
            case ScriptObject::NullType:
                goto bodyBreak;

            case ScriptObject::PairType:
                rest = static_cast<Pair*>(rcdr.data());
                break;

            default:
                throw ScriptError("Invalid body.", pair->position());
        }
    }
    bodyBreak:

    if (bytecode.isEmpty()) {
        throw ScriptError("Function has no body.", pair->position());
    }
    bytecode.append(ReturnOp);

    ScriptObjectPointer lambda(new Lambda(firstArgs, restArg,
        subscope.variableCount(), subscope.constants(), bytecode));
    out.append(LambdaOp, scope->addConstant(lambda));
    out.associate(pair->position());
}

/**
 * Compiles a quasiquote vector.
 */
static void compileQuasiquoteVector (Vector* vector, Scope* scope, Bytecode& out);

/**
 * Compiles a quasiquote pair.
 */
static void compileQuasiquotePair (Pair* pair, Scope* scope, Bytecode& out)
{
    out.append(ResetOperandCountOp);
    out.append(ConstantOp, scope->addConstant(appendProcedure()));

    for (Pair* rest = pair;; ) {
        ScriptObjectPointer car = rest->car();
        switch (car->type()) {
            case ScriptObject::PairType: {
                Pair* subpair = static_cast<Pair*>(car.data());
                ScriptObjectPointer subcar = subpair->car();
                if (subcar->type() == ScriptObject::SymbolType) {
                    Symbol* symbol = static_cast<Symbol*>(subcar.data());
                    if (symbol->name() == "unquote") {
                        Pair* subcdr = requirePair(subpair->cdr(),
                            "Invalid expression.", subpair->position());
                        requireNull(subcdr->cdr(), "Invalid expression.", subpair->position());
                        out.append(ResetOperandCountOp);
                        out.append(ConstantOp, scope->addConstant(listProcedure()));
                        compile(subcdr->car(), scope, false, false, false, out);
                        out.append(CallOp);
                        out.associate(subpair->position());
                        break;

                    } else if (symbol->name() == "unquote-splicing") {
                        Pair* subcdr = requirePair(subpair->cdr(),
                            "Invalid expression.", subpair->position());
                        requireNull(subcdr->cdr(), "Invalid expression.", subpair->position());
                        compile(subcdr->car(), scope, false, false, false, out);
                        break;
                    }
                }
                out.append(ResetOperandCountOp);
                out.append(ConstantOp, scope->addConstant(listProcedure()));
                compileQuasiquotePair(subpair, scope, out);
                out.append(CallOp);
                out.associate(subpair->position());
                break;
            }
            case ScriptObject::VectorType: {
                Vector* vector = static_cast<Vector*>(car.data());
                out.append(ResetOperandCountOp);
                out.append(ConstantOp, scope->addConstant(listProcedure()));
                compileQuasiquoteVector(vector, scope, out);
                out.append(CallOp);
                out.associate(vector->position());
                break;
            }
            default:
                out.append(ConstantOp, scope->addConstant(ScriptObjectPointer(
                    new Pair(car, Null::instance()))));
                break;
        }

        ScriptObjectPointer cdr = rest->cdr();
        switch (cdr->type()) {
            case ScriptObject::NullType:
                goto outerBreak;

            case ScriptObject::PairType:
                rest = static_cast<Pair*>(cdr.data());
                break;

            default:
                throw ScriptError("Invalid expression.", pair->position());
        }
    }
    outerBreak:

    out.append(CallOp);
    out.associate(pair->position());
}

static void compileQuasiquoteVector (Vector* vector, Scope* scope, Bytecode& out)
{
    out.append(ResetOperandCountOp);
    out.append(ConstantOp, scope->addConstant(vectorAppendProcedure()));

    foreach (const ScriptObjectPointer& element, vector->contents()) {
        switch (element->type()) {
            case ScriptObject::PairType: {
                Pair* subpair = static_cast<Pair*>(element.data());
                ScriptObjectPointer subcar = subpair->car();
                if (subcar->type() == ScriptObject::SymbolType) {
                    Symbol* symbol = static_cast<Symbol*>(subcar.data());
                    if (symbol->name() == "unquote") {
                        Pair* subcdr = requirePair(subpair->cdr(),
                            "Invalid expression.", subpair->position());
                        requireNull(subcdr->cdr(), "Invalid expression.", subpair->position());
                        out.append(ResetOperandCountOp);
                        out.append(ConstantOp, scope->addConstant(vectorProcedure()));
                        compile(subcdr->car(), scope, false, false, false, out);
                        out.append(CallOp);
                        out.associate(subpair->position());
                        break;

                    } else if (symbol->name() == "unquote-splicing") {
                        Pair* subcdr = requirePair(subpair->cdr(),
                            "Invalid expression.", subpair->position());
                        requireNull(subcdr->cdr(), "Invalid expression.", subpair->position());
                        compile(subcdr->car(), scope, false, false, false, out);
                        break;
                    }
                }
                out.append(ResetOperandCountOp);
                out.append(ConstantOp, scope->addConstant(vectorProcedure()));
                compileQuasiquotePair(subpair, scope, out);
                out.append(CallOp);
                out.associate(subpair->position());
                break;
            }
            case ScriptObject::VectorType: {
                Vector* vector = static_cast<Vector*>(element.data());
                out.append(ResetOperandCountOp);
                out.append(ConstantOp, scope->addConstant(vectorProcedure()));
                compileQuasiquoteVector(vector, scope, out);
                out.append(CallOp);
                out.associate(vector->position());
                break;
            }
            default:
                out.append(ConstantOp, scope->addConstant(ScriptObjectPointer(
                    new Vector(ScriptObjectPointerVector(1, element)))));
                break;
        }
    }

    out.append(CallOp);
    out.associate(vector->position());
}

static void compile (
    ScriptObjectPointer expr, Scope* scope, bool top, bool allowDef, bool tailCall, Bytecode& out)
{
    switch (expr->type()) {
        default: // datum
            compileDeferred(scope, top, out);
            out.append(ConstantOp, scope->addConstant(expr));
            maybeAppendPop(top, tailCall, out);
            return;

        case ScriptObject::VariableType: {
            compileDeferred(scope, top, out);
            Variable* variable = static_cast<Variable*>(expr.data());
            out.append(VariableOp, scope->depth() - variable->scopeDepth(), variable->index());
            maybeAppendPop(top, tailCall, out);
            return;
        }
        case ScriptObject::SyntaxRulesType:
            throw ScriptError("Invalid macro use.", ScriptPosition());

        case ScriptObject::IdentifierSyntaxType: {
            IdentifierSyntax* syntax = static_cast<IdentifierSyntax*>(expr.data());
            compile(syntax->generate(), scope, top, allowDef, tailCall, out);
            return;
        }
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            ScriptObjectPointer binding = scope->resolve(symbol->name());
            if (binding.isNull()) {
                throw ScriptError("Unresolved symbol.", symbol->position());
            }
            compile(binding, scope, top, allowDef, tailCall, out);
            return;
        }
        case ScriptObject::VectorType: {
            Vector* vector = static_cast<Vector*>(expr.data());
            throw ScriptError("Invalid expression.", vector->position());
        }
        case ScriptObject::NullType: {
            Null* null = static_cast<Null*>(expr.data());
            throw ScriptError("Invalid expression.", null->position());
        }
        case ScriptObject::PairType: {
            Pair* pair = static_cast<Pair*>(expr.data());
            ScriptObjectPointer car = pair->car();
            switch (car->type()) {
                case ScriptObject::SyntaxRulesType: {
                    SyntaxRules* syntax = static_cast<SyntaxRules*>(car.data());
                    ScriptObjectPointer transformed = syntax->maybeTransform(expr, scope);
                    if (transformed.isNull()) {
                        throw ScriptError("No pattern match.", pair->position());
                    }
                    compile(transformed, scope, top, allowDef, tailCall, out);
                    return;
                }
                case ScriptObject::SymbolType: {
                    Symbol* symbol = static_cast<Symbol*>(car.data());
                    const QString& name = symbol->name();
                    ScriptObjectPointer binding = scope->resolve(name);
                    if (binding.isNull()) {
                        if (name == "define-syntax") {
                            if (!(allowDef && out.isEmpty())) {
                                throw ScriptError("Syntax definitions not allowed here.",
                                    pair->position());
                            }
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid syntax definition.", pair->position());
                            Symbol* symbol = requireSymbol(cdr->car(),
                                "Invalid syntax definition.", pair->position());
                            Pair* cddr = requirePair(cdr->cdr(),
                                "Invalid syntax definition.", pair->position());
                            requireNull(cddr->cdr(),
                                "Invalid syntax definition.", pair->position());
                            scope->define(symbol->name(),
                                createMacroTransformer(cddr->car(), scope));

                        } else if (name == "let-syntax") {
                            compileSyntax(pair, scope, top, tailCall, allowDef, false, out);

                        } else if (name == "letrec-syntax") {
                            compileSyntax(pair, scope, top, tailCall, allowDef, true, out);

                        } else if (name == "define") {
                            if (!(allowDef && out.isEmpty())) {
                                throw ScriptError("Definitions not allowed here.",
                                    pair->position());
                            }
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid definition.", pair->position());
                            ScriptObjectPointer cadr = cdr->car();
                            ScriptObjectPointer member;
                            switch (cadr->type()) {
                                case ScriptObject::SymbolType: {
                                    Symbol* variable = static_cast<Symbol*>(cadr.data());
                                    ScriptObjectPointer cddr = cdr->cdr();
                                    switch (cddr->type()) {
                                        case ScriptObject::NullType:
                                            scope->addVariable(variable->name());
                                            break;

                                        case ScriptObject::PairType: {
                                            Pair* pcddr = static_cast<Pair*>(cddr.data());
                                            requireNull(pcddr->cdr(),
                                                "Invalid definition.", pair->position());
                                            scope->addVariable(variable->name(), pcddr->car());
                                            break;
                                        }
                                        default:
                                            throw ScriptError("Invalid definition.",
                                                pair->position());
                                    }
                                    break;
                                }
                                case ScriptObject::PairType: {
                                    Pair* apair = static_cast<Pair*>(cadr.data());
                                    Symbol* symbol = requireSymbol(apair->car(),
                                        "Invalid definition.", apair->position());
                                    scope->addVariable(symbol->name(), ScriptObjectPointer(new Pair(
                                        Symbol::instance("lambda"),
                                        ScriptObjectPointer(new Pair(
                                            apair->cdr(), cdr->cdr(), cdr->position())),
                                        pair->position())));
                                    break;
                                }
                                default:
                                    throw ScriptError("Invalid definition.", pair->position());
                            }
                        } else if (name == "lambda") {
                            compileDeferred(scope, top, out);
                            compileLambda(pair, scope, out);
                            maybeAppendPop(top, tailCall, out);

                        } else if (name == "set!") {
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid expression.", pair->position());
                            Pair* cddr = requirePair(cdr->cdr(),
                                "Invalid expression.", pair->position());
                            requireNull(cddr->cdr(), "Invalid expression.", pair->position());
                            ScriptObjectPointer cadr = cdr->car();
                            switch (cadr->type()) {
                                case ScriptObject::VariableType: {
                                    compileDeferred(scope, top, out);
                                    Variable* var = static_cast<Variable*>(cadr.data());
                                    compile(cddr->car(), scope, false, false, false, out);
                                    out.append(SetVariableOp,
                                        scope->depth() - var->scopeDepth(), var->index());
                                    out.append(ConstantOp, scope->addConstant(
                                        Unspecified::instance()));
                                    maybeAppendPop(top, tailCall, out);
                                    return;
                                }
                                case ScriptObject::SyntaxRulesType:
                                    throw ScriptError("Invalid macro use.", pair->position());

                                case ScriptObject::IdentifierSyntaxType: {
                                    IdentifierSyntax* syntax = static_cast<IdentifierSyntax*>(
                                        cadr.data());
                                    ScriptObjectPointer transformed = syntax->maybeTransform(
                                        cddr->car(), scope);
                                    if (transformed.isNull()) {
                                        throw ScriptError("No pattern match.", pair->position());
                                    }
                                    compile(transformed, scope, top, allowDef, tailCall, out);
                                    return;
                                }
                                case ScriptObject::SymbolType: {
                                    Symbol* var = static_cast<Symbol*>(cadr.data());
                                    ScriptObjectPointer binding = scope->resolve(var->name());
                                    if (binding.isNull()) {
                                        throw ScriptError("Unresolved symbol.", var->position());
                                    }
                                    switch (binding->type()) {
                                        case ScriptObject::VariableType: {
                                            compileDeferred(scope, top, out);
                                            Variable* variable =
                                                static_cast<Variable*>(binding.data());
                                            compile(cddr->car(), scope, false, false, false, out);
                                            out.append(SetVariableOp,
                                                scope->depth() - variable->scopeDepth(),
                                                variable->index());
                                            out.append(ConstantOp, scope->addConstant(
                                                Unspecified::instance()));
                                            maybeAppendPop(top, tailCall, out);
                                            return;
                                        }
                                        case ScriptObject::SyntaxRulesType:
                                            throw ScriptError("Invalid macro use.",
                                                pair->position());

                                        case ScriptObject::IdentifierSyntaxType: {
                                            IdentifierSyntax* syntax =
                                                static_cast<IdentifierSyntax*>(binding.data());
                                            ScriptObjectPointer transformed =
                                                syntax->maybeTransform(cddr->car(), scope);
                                            if (transformed.isNull()) {
                                                throw ScriptError("No pattern match.",
                                                    pair->position());
                                            }
                                            compile(transformed, scope, top,
                                                allowDef, tailCall, out);
                                            return;
                                        }
                                    }
                                }
                                default:
                                    throw ScriptError("Invalid expression.", pair->position());
                            }
                        } else if (name == "quote") {
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid expression.", pair->position());
                            requireNull(cdr->cdr(), "Invalid expression.", pair->position());
                            compileDeferred(scope, top, out);
                            out.append(ConstantOp, scope->addConstant(cdr->car()));
                            maybeAppendPop(top, tailCall, out);

                        } else if (name == "quasiquote") {
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid expression.", pair->position());
                            requireNull(cdr->cdr(), "Invalid expression.", pair->position());
                            compileDeferred(scope, top, out);
                            ScriptObjectPointer cadr = cdr->car();
                            switch (cadr->type()) {
                                case ScriptObject::PairType: {
                                    Pair* pair = static_cast<Pair*>(cadr.data());
                                    compileQuasiquotePair(pair, scope, out);
                                    break;
                                }
                                case ScriptObject::VectorType: {
                                    Vector* vector = static_cast<Vector*>(cadr.data());
                                    compileQuasiquoteVector(vector, scope, out);
                                    break;
                                }
                                default:
                                    out.append(ConstantOp, scope->addConstant(cadr));
                                    break;
                            }
                            maybeAppendPop(top, tailCall, out);

                        } else if (name == "if") {
                            Pair* cdr = requirePair(pair->cdr(),
                                "Invalid expression.", pair->position());
                            Pair* cddr = requirePair(cdr->cdr(),
                                "Invalid expression.", pair->position());
                            compileDeferred(scope, top, out);
                            compile(cdr->car(), scope, false, false, false, out);
                            Bytecode thencode;
                            compile(cddr->car(), scope, false, false, tailCall, thencode);
                            Bytecode elsecode;
                            ScriptObjectPointer cdddr = cddr->cdr();
                            switch (cdddr->type()) {
                                case ScriptObject::NullType:
                                    elsecode.append(ConstantOp, scope->addConstant(
                                        Unspecified::instance()));
                                    break;

                                case ScriptObject::PairType: {
                                    Pair* pcdddr = static_cast<Pair*>(cdddr.data());
                                    requireNull(pcdddr->cdr(),
                                        "Invalid expression.", pair->position());
                                    compile(pcdddr->car(), scope, false,
                                        false, tailCall, elsecode);
                                    break;
                                }
                                default:
                                    throw ScriptError("Invalid expression.", pair->position());
                            }
                            thencode.append(JumpOp, elsecode.length());
                            out.append(ConditionalJumpOp, thencode.length());
                            out.append(thencode);
                            out.append(elsecode);
                            maybeAppendPop(top, tailCall, out);

                        } else if (name == "begin") {
                            if (top) {
                                ScriptObjectPointer cdr = pair->cdr();
                                forever {
                                    switch (cdr->type()) {
                                        case ScriptObject::NullType:
                                            return;

                                        case ScriptObject::PairType: {
                                            Pair* pcdr = static_cast<Pair*>(cdr.data());
                                            compile(pcdr->car(), scope, true, allowDef,
                                                tailCall && pcdr->cdr()->type() ==
                                                    ScriptObject::NullType, out);
                                            cdr = pcdr->cdr();
                                            break;
                                        }
                                        default:
                                            throw ScriptError("Invalid expression.",
                                                pair->position());
                                    }
                                }
                            } else {
                                Pair* pcdr = requirePair(pair->cdr(),
                                    "Invalid expression.", pair->position());
                                forever {
                                    ScriptObjectPointer cddr = pcdr->cdr();
                                    switch (cddr->type()) {
                                        case ScriptObject::NullType:
                                            compile(pcdr->car(), scope, false, false, tailCall, out);
                                            return;

                                        case ScriptObject::PairType:
                                            compile(pcdr->car(), scope, false, false, false, out);
                                            out.append(PopOp);
                                            pcdr = static_cast<Pair*>(cddr.data());
                                            break;

                                        default:
                                            throw ScriptError("Invalid expression.",
                                                pair->position());
                                    }
                                }
                            }
                        } else {
                            throw ScriptError("Unresolved symbol.", symbol->position());
                        }
                        return;
                    }
                    switch (binding->type()) {
                        case ScriptObject::SyntaxRulesType: {
                            SyntaxRules* syntax = static_cast<SyntaxRules*>(binding.data());
                            ScriptObjectPointer transformed =
                                syntax->maybeTransform(expr, scope);
                            if (transformed.isNull()) {
                                throw ScriptError("No pattern match.", pair->position());
                            }
                            compile(transformed, scope, top, allowDef, tailCall, out);
                            return;
                        }
                    }
                }
            }
            compileDeferred(scope, top, out);
            out.append(ResetOperandCountOp);

            for (Pair* rest = pair;; ) {
                compile(rest->car(), scope, false, false, false, out);

                ScriptObjectPointer cdr = rest->cdr();
                switch (cdr->type()) {
                    case ScriptObject::NullType:
                        out.append(tailCall ? TailCallOp : CallOp);
                        out.associate(pair->position());
                        maybeAppendPop(top, tailCall, out);
                        return;

                    case ScriptObject::PairType:
                        rest = static_cast<Pair*>(cdr.data());
                        break;

                    default:
                        throw ScriptError("Invalid expression.", pair->position());
                }
            }
        }
    }
}

ScriptObjectPointer Evaluator::evaluateUntilExit (const QString& expr)
{
    compileForEvaluation(expr);
    return execute(_maxCyclesPerSlice = 0);
}

void Evaluator::evaluate (const QString& expr, int maxCyclesPerSlice)
{
    compileForEvaluation(expr);

    ScriptObjectPointer result = execute(_maxCyclesPerSlice = maxCyclesPerSlice);
    if (!result.isNull()) {
        emit exited(result);

    } else if (!_sleeping) {
        _timer.start(0);
    }
}

void Evaluator::interrupt ()
{
    _timer.stop();
}

ScriptObjectPointer Evaluator::execute (int maxCycles)
{
    // initialize state
    Invocation* invocation = static_cast<Invocation*>(
        _stack.at(_registers.invocationIdx).data());
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    const Bytecode* bytecode = &lambda->bytecode();

    while (maxCycles == 0 || maxCycles-- > 0) {
        switch (bytecode->charAt(_registers.instructionIdx++)) {
            case ConstantOp:
                _stack.push(lambda->constant(bytecode->intAt(_registers.instructionIdx)));
                _registers.instructionIdx += 4;
                _registers.operandCount++;
                break;

            case VariableOp:
                _stack.push(invocation->variable(
                    bytecode->intAt(_registers.instructionIdx),
                    bytecode->intAt(_registers.instructionIdx + 4)));
                _registers.instructionIdx += 8;
                _registers.operandCount++;
                break;

            case SetVariableOp:
                invocation->setVariable(
                    bytecode->intAt(_registers.instructionIdx),
                    bytecode->intAt(_registers.instructionIdx + 4), _stack.pop());
                _registers.instructionIdx += 8;
                _registers.operandCount--;
                break;

            case ResetOperandCountOp:
                _stack.push(Integer::instance(_registers.operandCount));
                _registers.operandCount = 0;
                break;

            CallOpLabel:
            case CallOp: {
                int pidx = _stack.size() - _registers.operandCount;
                ScriptObjectPointer sproc = _stack.at(pidx);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        ScriptObjectPointer* ocptr = _stack.data() + pidx - 1;
                        ScriptObjectPointer result;
                        try {
                            result = nproc->function()(
                                this, _registers.operandCount - 1, ocptr + 2);

                        } catch (const QString& message) {
                            throwScriptError(message);
                        }
                        if (result.isNull()) {
                            _sleeping = true;
                            _registers.operandCount = static_cast<Integer*>(
                                ocptr->data())->value();
                            _stack.resize(pidx - 1);
                            return ScriptObjectPointer();
                        }
                        _registers.operandCount = static_cast<Integer*>(
                            ocptr->data())->value() + 1;
                        *ocptr = result;
                        _stack.resize(pidx);
                        break;
                    }
                    case ScriptObject::ApplyProcedureType: {
                        if (_registers.operandCount < 3) {
                            throwScriptError("Requires at least two arguments.");
                        }
                        _stack.remove(pidx);
                        _registers.operandCount -= 2;
                        for (ScriptObjectPointer rest = _stack.pop();; ) {
                            switch (rest->type()) {
                                case ScriptObject::PairType: {
                                     Pair* pair = static_cast<Pair*>(rest.data());
                                     _stack.push(pair->car());
                                     _registers.operandCount++;
                                     rest = pair->cdr();
                                     break;
                                }
                                case ScriptObject::NullType:
                                    goto CallOpLabel;

                                default:
                                    throwScriptError("Invalid argument.");
                            }
                        }
                    }
                    case ScriptObject::CaptureProcedureType: {
                        if (_registers.operandCount != 2) {
                            throwScriptError("Requires exactly one argument.");
                        }
                        ScriptObjectPointer pproc = _stack.at(pidx + 1);
                        int ocidx = pidx - 1;
                        _registers.operandCount = static_cast<Integer*>(
                            _stack.at(ocidx).data())->value();
                        _stack.resize(ocidx);
                        ScriptObjectPointer eproc(new EscapeProcedure(_stack, _registers));
                        _collectable.append(eproc.toWeakRef());
                        _stack.push(Integer::instance(_registers.operandCount));
                        _stack.push(pproc);
                        _stack.push(eproc);
                        _registers.operandCount = 2;
                        goto CallOpLabel;
                    }
                    case ScriptObject::EscapeProcedureType: {
                        ScriptObjectPointerVector args = _stack.mid(pidx + 1,
                            _registers.operandCount - 1);
                        EscapeProcedure* eproc = static_cast<EscapeProcedure*>(sproc.data());
                        _stack = eproc->stack();
                        _registers = eproc->registers();
                        invocation = static_cast<Invocation*>(
                            _stack.at(_registers.invocationIdx).data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        _stack += args;
                        _registers.operandCount += args.size();
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        int nargs = _registers.operandCount - 1;
                        int ocidx = pidx - 1;
                        ScriptObjectPointer& ocref = _stack[ocidx];
                        Integer* oc = static_cast<Integer*>(ocref.data());
                        _registers.operandCount = oc->value();
                        ocref = ScriptObjectPointer(
                            invocation = new Invocation(sproc, _registers));
                        _collectable.append(ocref.toWeakRef());
                        proc = static_cast<LambdaProcedure*>(sproc.data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        ScriptObjectPointer* aptr = _stack.data() + pidx + 1;
                        ScriptObjectPointer* vptr = invocation->variables().data();
                        if (lambda->listArgument()) {
                            int ssize = lambda->scalarArgumentCount();
                            int lsize = nargs - ssize;
                            if (lsize < 0) {
                                throwScriptError("Too few arguments.");
                            }
                            for (ScriptObjectPointer* aend = aptr + ssize; aptr != aend; aptr++) {
                                *vptr++ = *aptr;
                            }
                            *vptr = listInstance(aptr, lsize);

                        } else {
                            if (nargs != lambda->scalarArgumentCount()) {
                                throwScriptError("Wrong number of arguments.");
                            }
                            for (ScriptObjectPointer* aend = aptr + nargs; aptr != aend; aptr++) {
                                *vptr++ = *aptr;
                            }
                        }
                        _stack.resize(pidx);
                        _registers.invocationIdx = ocidx;
                        _registers.instructionIdx = 0;
                        _registers.operandCount = 0;
                        break;
                    }
                    default:
                        throwScriptError("First operand not a procedure.");
                        break;
                }
                break;
            }
            TailCallOpLabel:
            case TailCallOp: {
                int pidx = _stack.size() - _registers.operandCount;
                ScriptObjectPointer sproc = _stack.at(pidx);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        ScriptObjectPointer* sdata = _stack.data();
                        ScriptObjectPointer result;
                        try {
                            result = nproc->function()(
                                this, _registers.operandCount - 1, sdata + pidx + 1);

                        } catch (const QString& message) {
                            throwScriptError(message);
                        }
                        int ridx = _registers.invocationIdx;
                        _registers = invocation->registers();
                        if (result.isNull()) {
                            _sleeping = true;
                            _stack.resize(ridx);
                            return ScriptObjectPointer();
                        }
                        invocation = static_cast<Invocation*>(
                            sdata[_registers.invocationIdx].data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        sdata[ridx] = result;
                        _stack.resize(ridx + 1);
                        _registers.operandCount++;
                        break;
                    }
                    case ScriptObject::ApplyProcedureType: {
                        if (_registers.operandCount < 3) {
                            throwScriptError("Requires at least two arguments.");
                        }
                        _stack.remove(pidx);
                        _registers.operandCount -= 2;
                        for (ScriptObjectPointer rest = _stack.pop();; ) {
                            switch (rest->type()) {
                                case ScriptObject::PairType: {
                                     Pair* pair = static_cast<Pair*>(rest.data());
                                     _stack.push(pair->car());
                                     _registers.operandCount++;
                                     rest = pair->cdr();
                                     break;
                                }
                                case ScriptObject::NullType:
                                    goto TailCallOpLabel;

                                default:
                                    throwScriptError("Invalid argument.");
                            }
                        }
                    }
                    case ScriptObject::CaptureProcedureType: {
                        if (_registers.operandCount != 2) {
                            throwScriptError("Requires exactly one argument.");
                        }
                        ScriptObjectPointer pproc = _stack.at(pidx + 1);
                        int ridx = _registers.invocationIdx;
                        _registers = invocation->registers();
                        invocation = static_cast<Invocation*>(
                            _stack.at(_registers.invocationIdx).data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        _stack.resize(ridx);
                        ScriptObjectPointer eproc(new EscapeProcedure(_stack, _registers));
                        _collectable.append(eproc.toWeakRef());
                        _stack.push(Integer::instance(_registers.operandCount));
                        _stack.push(pproc);
                        _stack.push(eproc);
                        _registers.operandCount = 2;
                        goto CallOpLabel;
                    }
                    case ScriptObject::EscapeProcedureType: {
                        ScriptObjectPointerVector args = _stack.mid(
                            pidx + 1, _registers.operandCount - 1);
                        EscapeProcedure* eproc = static_cast<EscapeProcedure*>(sproc.data());
                        _stack = eproc->stack();
                        _registers = eproc->registers();
                        invocation = static_cast<Invocation*>(
                            _stack.at(_registers.invocationIdx).data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        _stack += args;
                        _registers.operandCount += args.size();
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        int nargs = _registers.operandCount - 1;
                        ScriptObjectPointer iptr = ScriptObjectPointer(
                            invocation = new Invocation(sproc, invocation->registers()));
                        _collectable.append(iptr.toWeakRef());
                        proc = static_cast<LambdaProcedure*>(sproc.data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        ScriptObjectPointer* aptr = _stack.data() + pidx + 1;
                        ScriptObjectPointer* vptr = invocation->variables().data();
                        if (lambda->listArgument()) {
                            int ssize = lambda->scalarArgumentCount();
                            int lsize = nargs - ssize;
                            if (lsize < 0) {
                                throwScriptError("Too few arguments.");
                            }
                            for (ScriptObjectPointer* aend = aptr + ssize; aptr != aend; aptr++) {
                                *vptr++ = *aptr;
                            }
                            *vptr = listInstance(aptr, lsize);

                        } else {
                            if (nargs != lambda->scalarArgumentCount()) {
                                throwScriptError("Wrong number of arguments.");
                            }
                            for (ScriptObjectPointer* aend = aptr + nargs; aptr != aend; aptr++) {
                                *vptr++ = *aptr;
                            }
                        }
                        _stack[_registers.invocationIdx] = iptr;
                        _stack.resize(_registers.invocationIdx + 1);
                        _registers.instructionIdx = 0;
                        _registers.operandCount = 0;
                        break;
                    }
                    default:
                        throwScriptError("First operand not a procedure.");
                        break;
                }
                break;
            }
            case ReturnOp: {
                int ridx = _registers.invocationIdx;
                _registers = invocation->registers();
                invocation = static_cast<Invocation*>(_stack.at(_registers.invocationIdx).data());
                proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
                bytecode = &lambda->bytecode();
                _stack[ridx] = _stack.pop();
                _registers.operandCount++;
                break;
            }
            case LambdaOp: {
                ScriptObjectPointer nlambda = lambda->constant(
                    bytecode->intAt(_registers.instructionIdx));
                _registers.instructionIdx += 4;
                ScriptObjectPointer nproc(new LambdaProcedure(
                    nlambda, _stack.at(_registers.invocationIdx)));
                _collectable.append(nproc.toWeakRef());
                _stack.push(nproc);
                _registers.operandCount++;
                break;
            }
            case PopOp:
                _stack.pop();
                _registers.operandCount--;
                break;

            case ExitOp:
                _registers.operandCount--;
                return _stack.pop();

            case LambdaExitOp:
                return Unspecified::instance();

            case JumpOp:
                _registers.instructionIdx += bytecode->intAt(_registers.instructionIdx) + 4;
                break;

            case ConditionalJumpOp: {
                ScriptObjectPointer test = _stack.pop();
                if (test->type() == ScriptObject::BooleanType &&
                        static_cast<Boolean*>(test.data())->value()) {
                    _registers.instructionIdx += 4;
                } else {
                    _registers.instructionIdx += bytecode->intAt(_registers.instructionIdx) + 4;
                }
                break;
            }
        }
    }

    return ScriptObjectPointer();
}

void Evaluator::gc ()
{
    // mark everything on the stack with the next color in sequence
    _lastColor++;
    foreach (const ScriptObjectPointer& element, _stack) {
        element->mark(_lastColor);
    }

    // delete anything in our list that isn't marked
    for (QLinkedList<WeakScriptObjectPointer>::iterator it = _collectable.begin();
            it != _collectable.end(); it++) {
        ScriptObject* obj = it->data();
        if (obj == 0 || (!obj->sweep(_lastColor) && it->isNull())) {
            it = _collectable.erase(it);
        }
    }
}

ScriptObjectPointer Evaluator::listInstance (const ScriptObjectPointer* first, int count)
{
    if (count == 0) {
        return Null::instance();
    }
    ScriptObjectPointer head = pairInstance(*first), tail = head;
    for (const ScriptObjectPointer* it = first + 1, *end = first + count; it != end; it++) {
        ScriptObjectPointer pair = pairInstance(*it);
        static_cast<Pair*>(tail.data())->setCdr(pair);
        tail = pair;
    }
    static_cast<Pair*>(tail.data())->setCdr(Null::instance());
    return head;
}

ScriptObjectPointer Evaluator::pairInstance (
    const ScriptObjectPointer& car, const ScriptObjectPointer& cdr)
{
    ScriptObjectPointer instance(new Pair(car, cdr));
    _collectable.append(instance.toWeakRef());
    return instance;
}

ScriptObjectPointer Evaluator::vectorInstance (const ScriptObjectPointerVector& contents)
{
    ScriptObjectPointer instance = Vector::instance(contents);
    if (!contents.isEmpty()) {
        _collectable.append(instance.toWeakRef());
    }
    return instance;
}

void Evaluator::wakeUp (const ScriptObjectPointer& returnValue)
{
    _sleeping = false;
    _stack.push(returnValue);
    _registers.operandCount++;

    ScriptObjectPointer result = execute(_maxCyclesPerSlice);
    if (!result.isNull()) {
        emit exited(result);

    } else if (!_sleeping) {
        _timer.start(0);
    }
}

void Evaluator::continueExecuting ()
{
    ScriptObjectPointer result = execute(_maxCyclesPerSlice);
    if (!result.isNull()) {
        _timer.stop();
        emit exited(result);

    } else if (_sleeping) {
        _timer.stop();
    }
}

void Evaluator::compileForEvaluation (const QString& expr)
{
    Invocation* invocation = static_cast<Invocation*>(_scope.invocation().data());
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    _registers.instructionIdx = lambda->bytecode().length();

    Bytecode bytecode;
    bool result = false;
    Parser parser(expr, _source);
    ScriptObjectPointer datum;
    while (!(datum = parser.parse()).isNull()) {
        if (result) {
            lambda->bytecode().append(PopOp);
        }
        compile(datum, &_scope, false, true, false, bytecode);

        if (!(result = !bytecode.isEmpty())) {
            compileDeferred(&_scope, true, bytecode);
        }
        for (int ii = lambda->constants().size(), nn = _scope.constants().size(); ii < nn; ii++) {
            lambda->constants().append(_scope.constants().at(ii));
        }
        lambda->bytecode().append(bytecode);
        bytecode.clear();
    }
    lambda->bytecode().append(result ? ExitOp : LambdaExitOp);
}

void Evaluator::throwScriptError (const QString& message)
{
    QVector<ScriptPosition> positions;
    forever {
        Invocation* invocation = static_cast<Invocation*>(
            _stack.at(_registers.invocationIdx).data());
        LambdaProcedure* proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
        Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());

        positions.append(lambda->bytecode().position(_registers.instructionIdx));
        if (_registers.invocationIdx == 0) {
            break;
        }
        _registers = invocation->registers();
    }

    // clear the stack
    _stack.resize(1);
    _registers.operandCount = 0;

    throw ScriptError(message, positions);
}

Pair* requirePair (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position)
{
    if (obj->type() != ScriptObject::PairType) {
        throw ScriptError(message, position);
    }
    return static_cast<Pair*>(obj.data());
}

Symbol* requireSymbol (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position)
{
    if (obj->type() != ScriptObject::SymbolType) {
        throw ScriptError(message, position);
    }
    return static_cast<Symbol*>(obj.data());
}

Null* requireNull (const ScriptObjectPointer& obj, const QString& message,
    const ScriptPosition& position)
{
    if (obj->type() != ScriptObject::NullType) {
        throw ScriptError(message, position);
    }
    return static_cast<Null*>(obj.data());
}
