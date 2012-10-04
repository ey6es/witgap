//
// $Id$

#include <QtDebug>

#include "script/Evaluator.h"
#include "script/Globals.h"
#include "script/MacroTransformer.h"
#include "script/Parser.h"

Scope::Scope (Scope* parent, bool withValues, bool syntactic) :
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
    // first check the local bindings
    ScriptObjectPointer binding = _bindings.value(name);
    if (!binding.isNull() || _parent == 0) {
        return binding;
    }

    // try the parent, if any
    binding = _parent->resolve(name);
    if (binding.isNull() || _syntactic || binding->type() != ScriptObject::VariableType) {
        return binding;
    }
    Variable* variable = static_cast<Variable*>(binding.data());
    return ScriptObjectPointer(new Variable(variable->scope() + 1, variable->index()));
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
    ScriptObjectPointer variable(new Variable(0, idx));
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
    _lastColor(0)
{
    _stack.push(_scope.invocation());
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
    List* list, Scope* scope, bool top, bool allowDef, bool tailCall, bool rec, Bytecode& out)
{
    const ScriptObjectPointerList& contents = list->contents();
    int csize = contents.size();
    if (csize < 2) {
        throw ScriptError("Invalid syntax scope.", list->position());
    }
    ScriptObjectPointer bindings = contents.at(1);
    if (bindings->type() != ScriptObject::ListType) {
        throw ScriptError("Invalid syntax bindings.", list->position());
    }
    Scope subscope(scope, false, true);
    Scope* escope = rec ? &subscope : scope;
    List* blist = static_cast<List*>(bindings.data());
    foreach (const ScriptObjectPointer& binding, blist->contents()) {
        if (binding->type() != ScriptObject::ListType) {
            throw ScriptError("Invalid syntax bindings.", blist->position());
        }
        List* bpair = static_cast<List*>(binding.data());
        const ScriptObjectPointerList& bcontents = bpair->contents();
        if (bcontents.size() != 2) {
            throw ScriptError("Invalid syntax binding.", bpair->position());
        }
        ScriptObjectPointer car = bcontents.at(0);
        if (car->type() != ScriptObject::SymbolType) {
            throw ScriptError("Invalid syntax binding.", bpair->position());
        }
        Symbol* symbol = static_cast<Symbol*>(car.data());
        subscope.define(symbol->name(),
            createMacroTransformer(bcontents.at(1), escope));
    }
    for (int ii = 2; ii < csize; ii++) {
        compile(contents.at(ii), &subscope, top, allowDef, tailCall && ii == csize - 1, out);
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
    ScriptObjectPointerList& deferred = scope->deferred();
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
static void compileLambda (List* list, Scope* scope, Bytecode& out)
{
    const ScriptObjectPointerList& contents = list->contents();
    int csize = contents.size();
    if (csize < 3) {
        throw ScriptError("Invalid lambda.", list->position());
    }
    Scope subscope(scope);
    int firstArgs = 0;
    bool restArg = false;
    ScriptObjectPointer args = contents.at(1);
    switch (args->type()) {
        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(args.data());
            subscope.addVariable(symbol->name());
            restArg = true;
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
                if (rest) {
                    if (var->name() == "." || restArg) {
                        throw ScriptError("Invalid argument.", var->position());
                    }
                    subscope.addVariable(var->name());
                    restArg = true;
                    continue;
                }
                if (var->name() == ".") {
                    rest = true;
                } else {
                    subscope.addVariable(var->name());
                    firstArgs++;
                }
            }
            break;
        }
        default:
            Datum* datum = static_cast<Datum*>(args.data());
            throw ScriptError("Invalid formals.", datum->position());
    }
    Bytecode bytecode;
    for (int ii = 2; ii < csize; ii++) {
        compile(contents.at(ii), &subscope, true, true, ii == csize - 1, bytecode);
    }
    if (bytecode.isEmpty()) {
        throw ScriptError("Function has no body.", list->position());
    }
    bytecode.append(ReturnOp);
    
    ScriptObjectPointer lambda(new Lambda(firstArgs, restArg,
        subscope.variableCount(), subscope.constants(), bytecode));
    out.append(LambdaOp, scope->addConstant(lambda));
    out.associate(list->position());
}

/**
 * Compiles a quasiquote form.
 */
static void compileQuasiquote (List* list, Scope* scope, Bytecode& out)
{
    out.append(ResetOperandCountOp);
    out.append(ConstantOp, scope->addConstant(appendProcedure()));
    foreach (const ScriptObjectPointer& element, list->contents()) {
        if (element->type() != ScriptObject::ListType) {
            out.append(ConstantOp, scope->addConstant(List::instance(element)));
            continue;
        }
        List* sublist = static_cast<List*>(element.data());
        const ScriptObjectPointerList& subcontents = sublist->contents();
        int csize = subcontents.size();
        if (csize == 0) {
            out.append(ConstantOp, scope->addConstant(List::instance(element)));
            continue;
        }
        ScriptObjectPointer car = subcontents.at(0);
        if (car->type() == ScriptObject::SymbolType) {
            Symbol* symbol = static_cast<Symbol*>(car.data());
            const QString& name = symbol->name();
            if (name == "unquote") {
                if (csize != 2) {
                    throw ScriptError("Invalid expression.", sublist->position());
                }
                out.append(ResetOperandCountOp);
                out.append(ConstantOp, scope->addConstant(listProcedure()));
                compile(subcontents.at(1), scope, false, false, false, out);
                out.append(CallOp);
                out.associate(sublist->position());
                continue;
                
            } else if (name == "unquote-splicing") {
                if (csize != 2) {
                    throw ScriptError("Invalid expression.", sublist->position());
                }
                compile(subcontents.at(1), scope, false, false, false, out);
                continue;
            }
        }
        out.append(ResetOperandCountOp);
        out.append(ConstantOp, scope->addConstant(listProcedure()));
        compileQuasiquote(sublist, scope, out);
        out.append(CallOp);
        out.associate(sublist->position());
    }
    out.append(CallOp);
    out.associate(list->position());
}

static void compile (
    ScriptObjectPointer expr, Scope* scope, bool top, bool allowDef, bool tailCall, Bytecode& out)
{
    switch (expr->type()) {
        case ScriptObject::BooleanType:
        case ScriptObject::IntegerType:
        case ScriptObject::FloatType:
        case ScriptObject::StringType:
            compileDeferred(scope, top, out);
            out.append(ConstantOp, scope->addConstant(expr));
            maybeAppendPop(top, tailCall, out);
            return;
            
        case ScriptObject::VariableType: {
            compileDeferred(scope, top, out);
            Variable* variable = static_cast<Variable*>(expr.data());
            out.append(VariableOp, variable->scope(), variable->index());
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
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const ScriptObjectPointerList& contents = list->contents();
            if (contents.isEmpty()) {
                throw ScriptError("Invalid expression.", list->position());
            }
            ScriptObjectPointer car = contents.at(0);
            switch (car->type()) {
                case ScriptObject::SyntaxRulesType: {
                    SyntaxRules* syntax = static_cast<SyntaxRules*>(car.data());
                    ScriptObjectPointer transformed = syntax->maybeTransform(expr, scope);
                    if (transformed.isNull()) {
                        throw ScriptError("No pattern match.", list->position());
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
                                    list->position());
                            }
                            if (contents.size() != 3) {
                                throw ScriptError("Invalid syntax definition.", list->position());
                            }
                            ScriptObjectPointer cadr = contents.at(1);
                            if (cadr->type() != ScriptObject::SymbolType) {
                                throw ScriptError("Invalid syntax definition.", list->position());
                            }
                            Symbol* symbol = static_cast<Symbol*>(cadr.data());
                            scope->define(symbol->name(),
                                createMacroTransformer(contents.at(2), scope));
                                
                        } else if (name == "let-syntax") {
                            compileSyntax(list, scope, top, tailCall, allowDef, false, out);
                        
                        } else if (name == "letrec-syntax") {
                            compileSyntax(list, scope, top, tailCall, allowDef, true, out);
                            
                        } else if (name == "define") {
                            if (!(allowDef && out.isEmpty())) {
                                throw ScriptError("Definitions not allowed here.",
                                    list->position());
                            }
                            int csize = contents.size();
                            if (csize < 2) {
                                throw ScriptError("Invalid definition.", list->position());
                            }
                            ScriptObjectPointer cadr = contents.at(1);
                            ScriptObjectPointer member;
                            switch (cadr->type()) {
                                case ScriptObject::SymbolType: {
                                    Symbol* variable = static_cast<Symbol*>(cadr.data());
                                    if (csize == 2) {
                                        scope->addVariable(variable->name());

                                    } else if (csize == 3) {
                                        scope->addVariable(variable->name(), contents.at(2));
                                        
                                    } else {
                                        throw ScriptError("Invalid definition.", list->position());
                                    }
                                    break;
                                }
                                case ScriptObject::ListType: {
                                    List* alist = static_cast<List*>(cadr.data());
                                    const ScriptObjectPointerList& acontents = alist->contents();
                                    int acsize = acontents.size();
                                    if (acsize == 0) {
                                        throw ScriptError("Invalid definition.",
                                            alist->position());
                                    }
                                    ScriptObjectPointer car = acontents.at(0);
                                    if (car->type() != ScriptObject::SymbolType) {
                                        throw ScriptError("Invalid definition.",
                                            alist->position());
                                    }
                                    Symbol* symbol = static_cast<Symbol*>(car.data());
                                    ScriptObjectPointerList lcontents;
                                    lcontents.append(Symbol::instance("lambda"));
                                    ScriptObjectPointerList ncontents;
                                    for (int ii = 1; ii < acsize; ii++) {
                                        ncontents.append(acontents.at(ii));
                                    }
                                    lcontents.append(ScriptObjectPointer(
                                        new List(ncontents, alist->position())));
                                    for (int ii = 2; ii < csize; ii++) {
                                        lcontents.append(contents.at(ii));
                                    }
                                    scope->addVariable(symbol->name(), ScriptObjectPointer(
                                        new List(lcontents, list->position())));
                                    break;
                                }
                                default:
                                    throw ScriptError("Invalid definition.", list->position());
                            }           
                        } else if (name == "lambda") {
                            compileDeferred(scope, top, out);
                            compileLambda(list, scope, out);
                            maybeAppendPop(top, tailCall, out);
                            
                        } else if (name == "set!") {
                            if (contents.size() != 3) {
                                throw ScriptError("Invalid expression.", list->position());
                            }
                            ScriptObjectPointer variable = contents.at(1);
                            switch (variable->type()) {
                                case ScriptObject::VariableType: {
                                    compileDeferred(scope, top, out);
                                    Variable* var = static_cast<Variable*>(variable.data());
                                    compile(contents.at(2), scope, false, false, false, out);
                                    out.append(SetVariableOp, var->scope(), var->index());
                                    out.append(ConstantOp, scope->addConstant(
                                        Unspecified::instance()));
                                    maybeAppendPop(top, tailCall, out);
                                    return;
                                }
                                case ScriptObject::SyntaxRulesType:
                                    throw ScriptError("Invalid macro use.", list->position());
                                    
                                case ScriptObject::IdentifierSyntaxType: {
                                    IdentifierSyntax* syntax = static_cast<IdentifierSyntax*>(
                                        variable.data());
                                    ScriptObjectPointer transformed = syntax->maybeTransform(
                                        contents.at(2), scope);
                                    if (transformed.isNull()) {
                                        throw ScriptError("No pattern match.", list->position());
                                    }
                                    compile(transformed, scope, top, allowDef, tailCall, out);
                                    return;
                                }
                                case ScriptObject::SymbolType: {
                                    Symbol* var = static_cast<Symbol*>(variable.data());
                                    ScriptObjectPointer binding = scope->resolve(var->name());
                                    if (binding.isNull()) {
                                        throw ScriptError("Unresolved symbol.", var->position());
                                    }
                                    switch (binding->type()) {
                                        case ScriptObject::VariableType: {
                                            compileDeferred(scope, top, out);
                                            Variable* variable = static_cast<Variable*>(binding.data());
                                            compile(contents.at(2), scope, false,
                                                false, false, out);
                                            out.append(SetVariableOp, variable->scope(),
                                                variable->index());
                                            out.append(ConstantOp, scope->addConstant(
                                                Unspecified::instance()));
                                            maybeAppendPop(top, tailCall, out);
                                            return;
                                        }
                                        case ScriptObject::SyntaxRulesType:
                                            throw ScriptError("Invalid macro use.",
                                                list->position());
                                            
                                        case ScriptObject::IdentifierSyntaxType: {
                                            IdentifierSyntax* syntax =
                                                static_cast<IdentifierSyntax*>(binding.data());
                                            ScriptObjectPointer transformed =
                                                syntax->maybeTransform(contents.at(2), scope);
                                            if (transformed.isNull()) {
                                                throw ScriptError("No pattern match.",
                                                    list->position());
                                            }
                                            compile(transformed, scope, top,
                                                allowDef, tailCall, out);
                                            return;
                                        }
                                    }
                                }
                                default:
                                    throw ScriptError("Invalid expression.", list->position());
                            }
                        } else if (name == "quote") {
                            if (contents.size() != 2) {
                                throw ScriptError("Invalid expression.", list->position());
                            }
                            compileDeferred(scope, top, out);
                            out.append(ConstantOp, scope->addConstant(contents.at(1)));
                            maybeAppendPop(top, tailCall, out);
                        
                        } else if (name == "quasiquote") {
                            if (contents.size() != 2) {
                                throw ScriptError("Invalid expression.", list->position());
                            }
                            compileDeferred(scope, top, out);
                            const ScriptObjectPointer& cadr = contents.at(1);
                            if (cadr->type() != ScriptObject::ListType) {
                                out.append(ConstantOp, scope->addConstant(cadr));
                            } else {
                                List* list = static_cast<List*>(cadr.data());
                                compileQuasiquote(list, scope, out);
                            }
                            maybeAppendPop(top, tailCall, out);
                            
                        } else if (name == "if") {
                            int csize = contents.size();
                            if (csize != 3 && csize != 4) {
                                throw ScriptError("Invalid expression.", list->position());
                            }
                            compileDeferred(scope, top, out);
                            compile(contents.at(1), scope, false, false, false, out);
                            Bytecode thencode;
                            compile(contents.at(2), scope, false, false, tailCall, thencode);
                            Bytecode elsecode;
                            if (csize == 4) {
                                compile(contents.at(3), scope, false, false, tailCall, elsecode);
                            } else {
                                elsecode.append(ConstantOp, scope->addConstant(
                                    Unspecified::instance()));
                            }
                            thencode.append(JumpOp, elsecode.length());
                            out.append(ConditionalJumpOp, thencode.length());
                            out.append(thencode);
                            out.append(elsecode);
                            maybeAppendPop(top, tailCall, out);
                            
                        } else if (name == "begin") {
                            int csize = contents.size();
                            if (top) {
                                for (int ii = 1; ii < csize; ii++) {
                                    compile(contents.at(ii), scope, true, allowDef,
                                        tailCall && ii == csize - 1, out);
                                }
                            } else {
                                if (csize == 1) {
                                    throw ScriptError("Invalid expression.", list->position());
                                }
                                int lastIdx = csize - 1;
                                for (int ii = 1; ii < lastIdx; ii++) {
                                    compile(contents.at(ii), scope, false, false, false, out);
                                    out.append(PopOp);
                                }
                                compile(contents.at(lastIdx), scope, false, false, tailCall, out);
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
                                throw ScriptError("No pattern match.", list->position());
                            }
                            compile(transformed, scope, top, allowDef, tailCall, out);
                            return;     
                        }
                    }
                }
            }
            compileDeferred(scope, top, out);
            out.append(ResetOperandCountOp);
            foreach (const ScriptObjectPointer& operand, contents) {
                compile(operand, scope, false, false, false, out);
            }
            out.append(tailCall ? TailCallOp : CallOp);
            out.associate(list->position());
            maybeAppendPop(top, tailCall, out);
            return;
        }
    }
}

ScriptObjectPointer Evaluator::evaluate (const QString& expr)
{
    Parser parser(expr, _source);
    ScriptObjectPointer datum;
    ScriptObjectPointer result;
    Invocation* invocation = static_cast<Invocation*>(_scope.invocation().data());
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    Bytecode bytecode;    
    while (!(datum = parser.parse()).isNull()) {
        compile(datum, &_scope, false, true, false, bytecode);
        
        if (bytecode.isEmpty()) {
            compileDeferred(&_scope, true, bytecode);
            bytecode.append(LambdaExitOp);
            
        } else {
            bytecode.append(ExitOp);
        }
        for (int ii = lambda->constants().size(), nn = _scope.constants().size(); ii < nn; ii++) {
            lambda->constants().append(_scope.constants().at(ii));
        }
        _registers.instructionIdx = lambda->bytecode().length();
        lambda->bytecode().append(bytecode);
        bytecode.clear();
    
        result = execute();
    }
    return result;
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
                        _registers.operandCount = static_cast<Integer*>(
                            ocptr->data())->value() + 1;
                        *ocptr = result;
                        _stack.resize(pidx);
                        break;
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
                        if (_registers.operandCount != 2) {
                            throwScriptError("Requires exactly one argument.");
                        }
                        ScriptObjectPointer arg = _stack.at(pidx + 1);
                        EscapeProcedure* eproc = static_cast<EscapeProcedure*>(sproc.data());
                        _stack = eproc->stack();
                        _registers = eproc->registers();
                        invocation = static_cast<Invocation*>(
                            _stack.at(_registers.invocationIdx).data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        _stack.push(arg);
                        _registers.operandCount++;
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
                            ScriptObjectPointerList contents;
                            for (ScriptObjectPointer* aend = aptr + lsize; aptr != aend; aptr++) {
                                contents.append(*aptr);
                            }
                            *vptr = listInstance(contents);

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
                        if (_registers.operandCount != 2) {
                            throwScriptError("Requires exactly one argument.");
                        }
                        ScriptObjectPointer arg = _stack.at(pidx + 1);
                        EscapeProcedure* eproc = static_cast<EscapeProcedure*>(sproc.data());
                        _stack = eproc->stack();
                        _registers = eproc->registers();
                        invocation = static_cast<Invocation*>(
                            _stack.at(_registers.invocationIdx).data());
                        proc = static_cast<LambdaProcedure*>(invocation->procedure().data());
                        lambda = static_cast<Lambda*>(proc->lambda().data());
                        bytecode = &lambda->bytecode();
                        _stack.push(arg);
                        _registers.operandCount++;
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
                            ScriptObjectPointerList contents;
                            for (ScriptObjectPointer* aend = aptr + lsize; aptr != aend; aptr++) {
                                contents.append(*aptr);
                            }
                            *vptr = listInstance(contents);

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

ScriptObjectPointer Evaluator::listInstance (const ScriptObjectPointerList& contents)
{
    ScriptObjectPointer instance = List::instance(contents);
    if (!contents.isEmpty()) {
        _collectable.append(instance.toWeakRef());
    }
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
