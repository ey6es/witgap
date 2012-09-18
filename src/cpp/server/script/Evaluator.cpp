//
// $Id$

#include <QStringList>
#include <QtDebug>
#include <QtEndian>

#include "script/Evaluator.h"
#include "script/Globals.h"
#include "script/MacroTransformer.h"
#include "script/Parser.h"

Scope::Scope (Scope* parent, bool withValues, bool syntactic) :
    _parent(parent),
    _syntactic(syntactic),
    _memberCount(0),
    _lambdaProc(withValues ? new LambdaProcedure(ScriptObjectPointer(new Lambda()),
        parent == 0 ? ScriptObjectPointer() : parent->_lambdaProc) : 0)
{
}

void Scope::setArguments (const QStringList& firstArgs, const QString& restArg)
{
    int idx = 0;
    foreach (const QString& arg, firstArgs) {
        define(arg, ScriptObjectPointer(new Argument(idx++)));
    }
    if (!restArg.isEmpty()) {
        define(restArg, ScriptObjectPointer(new Argument(idx++)));
    }
}

ScriptObjectPointer Scope::resolve (const QString& name)
{
    // first check the local variables
    ScriptObjectPointer variable = _variables.value(name);
    if (!variable.isNull()) {
        return variable;
    }

    // try the parent, if any
    if (_parent == 0) {
        return ScriptObjectPointer();
    }
    ScriptObjectPointer pbinding = _parent->resolve(name);
    if (_syntactic || pbinding.isNull()) {
        return pbinding; // syntactic scopes do not modify
    }
    switch (pbinding->type()) {
        case ScriptObject::ArgumentType: {
            // copy parent argument to local member on initialization
            Argument* argument = static_cast<Argument*>(pbinding.data());
            _initBytecode.append(ArgumentOp, argument->index());
            _initBytecode.append(SetMemberOp, 0, _memberCount);
            return addMember(name);
        }
        case ScriptObject::MemberType: {
            Member* member = static_cast<Member*>(pbinding.data());
            return ScriptObjectPointer(new Member(member->scope() + 1, member->index()));
        }
        default:
            return pbinding;
    }
}

ScriptObjectPointer Scope::addMember (const QString& name, NativeProcedure::Function function)
{
    return addMember(name, ScriptObjectPointer(new NativeProcedure(function)));
}

ScriptObjectPointer Scope::addMember (const QString& name, const ScriptObjectPointer& value)
{
    if (_syntactic) { // syntactic scopes do not contain members
        return _parent->addMember(name, value);
    }
    int idx = _memberCount++;
    ScriptObjectPointer member(new Member(0, idx));
    define(name, member);
    if (!_lambdaProc.isNull()) {
        LambdaProcedure* proc = static_cast<LambdaProcedure*>(_lambdaProc.data());
        proc->appendMember(value);
    }
    return member;
}

void Scope::define (const QString& name, const ScriptObjectPointer& value)
{
    _variables.insert(name, value);
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
    _registers()
{
    _stack.push(_scope.lambdaProc());
}

/**
 * Expands the specified parsed expression in the given scope, appending the result to the
 * provided list.
 *
 * @param top if true, this is a top-level form (or begin within a top-level form, etc.)
 * @param expanded if true, the form is already expanded (so, just process it).
 */
static void expand (
    ScriptObjectPointer expr, Scope* scope, bool top, bool expanded, ScriptObjectPointerList& out);

/**
 * Makes sure that all definitions have been expanded, because we are about to expand a body
 * expression.
 */
static void ensureDefsExpanded (Scope* scope, bool top, ScriptObjectPointerList& out)
{
    QList<ScriptObjectPointerBoolPair>& defs = scope->defines();
    if (!top || defs.isEmpty()) {
        return;
    }
    foreach (const ScriptObjectPointerBoolPair& def, defs) {
        if (def.second) {
            out.append(def.first);
        } else {
            expand(def.first, scope, false, false, out);
        }
    }
    defs.clear();
}

/**
 * Expands a lambda expression/function definition.
 */
static void expandLambda (List* list, Scope* scope, ScriptObjectPointerList& out)
{
    const ScriptObjectPointerList& contents = list->contents();
    int csize = contents.size();
    if (csize < 3) {
        throw ScriptError("Invalid lambda.", list->position());
    }
    Scope subscope(scope);
    ScriptObjectPointerList lcontents;
    lcontents.append(contents.at(0));
    lcontents.append(contents.at(1));
    for (int ii = 2; ii < csize; ii++) {
        expand(contents.at(ii), &subscope, true, false, lcontents);
    }
    ensureDefsExpanded(&subscope, true, lcontents);
    out.append(ScriptObjectPointer(new List(lcontents, list->position())));
}

/**
 * Expands a let-syntax or letrec-syntax form.
 */
static void expandSyntax (
    List* list, Scope* scope, bool top, bool rec, ScriptObjectPointerList& out)
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
        ScriptObjectPointerList ncontents;
        expand(bcontents.at(1), escope, false, false, ncontents);
        subscope.define(symbol->name(),
            createMacroTransformer(ncontents.at(0), escope));
    }
    for (int ii = 2; ii < csize; ii++) {
        expand(contents.at(ii), &subscope, top, false, out);
    }
}

static void expand (
    ScriptObjectPointer expr, Scope* scope, bool top, bool expanded, ScriptObjectPointerList& out)
{
    switch (expr->type()) {
        case ScriptObject::SymbolType: {
            if (expanded) {
                break;
            }
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            const QString& name = symbol->name();
            ScriptObjectPointer binding = scope->resolve(name);
            if (!binding.isNull()) {
                switch (binding->type()) {
                    case ScriptObject::SyntaxRulesType:
                        throw ScriptError("Invalid macro use.", symbol->position());
                        
                    case ScriptObject::IdentifierSyntaxType: {
                        IdentifierSyntax* syntax = static_cast<IdentifierSyntax*>(binding.data());
                        expand(syntax->generate(), scope, top, true, out);
                        return;
                    }
                }
            }
            break;
        }
        case ScriptObject::ListType: {
            List* list = static_cast<List*>(expr.data());
            const ScriptObjectPointerList& contents = list->contents();
            int csize = contents.size();
            if (csize == 0) {
                break;
            }
            ScriptObjectPointer car = contents.at(0);
            if (car->type() == ScriptObject::SymbolType) {
                Symbol* symbol = static_cast<Symbol*>(car.data());
                const QString& name = symbol->name();
                if (!expanded) {
                    ScriptObjectPointer binding = scope->resolve(name);
                    if (!binding.isNull()) {
                        switch (binding->type()) {
                            case ScriptObject::SyntaxRulesType: {
                                ScriptObjectPointerList ncontents;
                                ncontents.append(car);
                                for (int ii = 1; ii < csize; ii++) {
                                    expand(contents.at(ii), scope, false, false, ncontents);
                                }
                                SyntaxRules* syntax = static_cast<SyntaxRules*>(binding.data());
                                ScriptObjectPointer transformed = syntax->maybeTransform(
                                    ScriptObjectPointer(new List(
                                        ncontents, list->position())), scope);
                                if (transformed.isNull()) {
                                    throw ScriptError("No pattern match.", list->position());
                                }
                                expand(transformed, scope, top, true, out);
                                return;
                            }
                            case ScriptObject::IdentifierSyntaxType:
                                throw ScriptError("Invalid macro use.", symbol->position());
                        }
                    }
                }
                if (name == "define-syntax") {
                    if (contents.size() != 3) {
                        throw ScriptError("Invalid syntax definition.", list->position());
                    }
                    if (top) {
                        ScriptObjectPointer cadr = contents.at(1);
                        if (cadr->type() != ScriptObject::SymbolType) {
                            throw ScriptError("Invalid syntax definition.", list->position());
                        }
                        Symbol* symbol = static_cast<Symbol*>(cadr.data());
                        if (expanded) {
                            scope->define(symbol->name(),
                                createMacroTransformer(contents.at(2), scope));
                            return;
                        }
                        ScriptObjectPointerList ncontents;
                        expand(contents.at(2), scope, false, false, ncontents);
                        scope->define(symbol->name(),
                            createMacroTransformer(ncontents.at(0), scope));
                        return;
                    }
                    if (expanded) {
                        break;
                    }
                    ScriptObjectPointerList ncontents;
                    ncontents.append(car);
                    ncontents.append(contents.at(1));
                    expand(contents.at(2), scope, false, false, ncontents);
                    out.append(ScriptObjectPointer(new List(ncontents, list->position())));
                    return;

                } else if (name == "define") {
                    if (csize < 2) {
                        throw ScriptError("Invalid definition.", list->position());
                    }
                    ScriptObjectPointer cadr = contents.at(1);
                    if (top) {
                        switch (cadr->type()) {
                            case ScriptObject::SymbolType: {
                                Symbol* symbol = static_cast<Symbol*>(cadr.data());
                                scope->addMember(symbol->name());
                                break;
                            }
                            case ScriptObject::ListType: {
                                List* list = static_cast<List*>(cadr.data());
                                const ScriptObjectPointerList& lcontents = list->contents();
                                if (lcontents.isEmpty() ||
                                        lcontents.at(0)->type() != ScriptObject::SymbolType) {
                                    throw ScriptError("Invalid definition.", list->position());
                                }
                                Symbol* symbol = static_cast<Symbol*>(lcontents.at(0).data());
                                scope->addMember(symbol->name());
                                break;
                            }
                        }
                        scope->defines().append(ScriptObjectPointerBoolPair(expr, expanded));
                        return;
                    }
                    if (expanded) {
                        break;
                    }
                    switch (cadr->type()) {
                        case ScriptObject::SymbolType:
                            if (csize == 2) {
                                out.append(expr);
                                
                            } else {
                                ScriptObjectPointerList ncontents;
                                ncontents.append(car);
                                ncontents.append(cadr);
                                expand(contents.at(2), scope, false, false, ncontents);
                                out.append(ScriptObjectPointer(new List(
                                    ncontents, list->position())));
                            }
                            return;
                        
                        case ScriptObject::ListType:
                            expandLambda(list, scope, out);
                            return;
                    }
                    
                } else if (name == "set!") {
                    if (expanded) {
                        break;
                    }
                    if (csize != 3) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    ScriptObjectPointer variable = contents.at(1);
                    if (variable->type() != ScriptObject::SymbolType) {
                        Datum* datum = static_cast<Datum*>(variable.data());
                        throw ScriptError("Not a variable.", datum->position());
                    }
                    Symbol* var = static_cast<Symbol*>(variable.data());
                    ScriptObjectPointer binding = scope->resolve(var->name());
                    if (!binding.isNull()) {
                        switch (binding->type()) {
                            case ScriptObject::SyntaxRulesType:
                                throw ScriptError("Invalid macro use.", list->position());
                                
                            case ScriptObject::IdentifierSyntaxType: {
                                ScriptObjectPointerList ncontents;
                                expand(contents.at(2), scope, false, false, ncontents);
                                IdentifierSyntax* syntax = static_cast<IdentifierSyntax*>(
                                    binding.data());
                                ScriptObjectPointer transformed = syntax->maybeTransform(
                                    ncontents.at(0), scope);
                                if (transformed.isNull()) {
                                    throw ScriptError("No pattern match.", list->position());
                                }
                                expand(transformed, scope, top, true, out);
                                return;
                            }
                        }
                    }
                } else if (name == "lambda") {
                    if (expanded) {
                        break;
                    }
                    ensureDefsExpanded(scope, top, out);
                    expandLambda(list, scope, out);
                    return;
                
                } else if (name == "quote") {
                    break;
                    
                } else if (name == "let-syntax") {
                    expandSyntax(list, scope, top, false, out);
                    return;
                    
                } else if (name == "letrec-syntax") {
                    expandSyntax(list, scope, top, true, out);
                    return;
                    
                } else if (name == "begin" && top) {
                    // splice in the forms
                    for (int ii = 1; ii < csize; ii++) {
                        expand(contents.at(ii), scope, true, false, out);
                    }
                    return;
                }
            }
            if (expanded) {
                break;
            }
            ensureDefsExpanded(scope, top, out);
            ScriptObjectPointerList ncontents;
            for (int ii = 0; ii < csize; ii++) {
                expand(contents.at(ii), scope, false, false, ncontents);
            }
            out.append(ScriptObjectPointer(new List(ncontents, list->position())));
            return;
        }
    }
    ensureDefsExpanded(scope, top, out);
    out.append(expr);
}

/**
 * Compiles the specified parsed expression in the given scope to the supplied buffer.
 *
 * @param allowDef whether or not to allow a definition.
 * @param tailCall whether or not this is a tail call context.
 */
static void compile (
    ScriptObjectPointer expr, Scope* scope, bool allowDef, bool tailCall, Bytecode& out);

/**
 * Compiles the body of a lambda/function definition and optionally returns the defined Member.
 *
 * @param define if true, we're defining a member in the current scope; return the Member.
 */
static ScriptObjectPointer compileLambda (
    List* list, Scope* scope, Bytecode& out, bool define = false)
{
    const QList<ScriptObjectPointer>& contents = list->contents();
    if (contents.size() < 3) {
        throw ScriptError("Invalid lambda.", list->position());
    }
    ScriptObjectPointer args = contents.at(1);
    ScriptObjectPointer member;
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
                if (define && member.isNull()) {
                    member = scope->addMember(var->name());
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
    // make sure we have a member if necessary
    if (define && member.isNull()) {
        throw ScriptError("Invalid definition.", list->position());
    }
    Scope subscope(scope);
    Bytecode bodyBytecode;
    for (int ii = 2, nn = contents.size(); ii < nn; ii++) {
        ScriptObjectPointer element = contents.at(ii);
        if (bodyBytecode.isEmpty()) {
            if (element->type() == ScriptObject::ListType) {
                // peek in to see if it's a define
                List* list = static_cast<List*>(element.data());
                if (!list->contents().isEmpty()) {
                    ScriptObjectPointer car = list->contents().at(0);
                    if (car->type() == ScriptObject::SymbolType) {
                        Symbol* symbol = static_cast<Symbol*>(car.data());
                        if (symbol->name() == "define") {
                            compile(element, &subscope, true, false, bodyBytecode);
                            continue;
                        }
                    }
                }
            }
            // install the arguments now that the defines have ended
            subscope.setArguments(firstArgs, restArg);

        } else {
            bodyBytecode.append(PopOp);
        }
        compile(element, &subscope, false, ii == nn - 1, bodyBytecode);
    }
    if (bodyBytecode.isEmpty()) {
        throw ScriptError("Function has no body.", list->position());
    }
    subscope.initBytecode().append(LambdaReturnOp);
    bodyBytecode.append(ReturnOp);
    
    int bodyIdx = subscope.initBytecode().length();
    subscope.initBytecode().append(bodyBytecode);
    ScriptObjectPointer lambda(new Lambda(firstArgs.size(), !restArg.isEmpty(),
        subscope.memberCount(), subscope.constants(), subscope.initBytecode(),
        bodyIdx));
    out.append(LambdaOp, scope->addConstant(lambda));
    out.associate(list->position());
    return member;
}

static void compile (
    ScriptObjectPointer expr, Scope* scope, bool allowDef, bool tailCall, Bytecode& out)
{
    switch (expr->type()) {
        case ScriptObject::BooleanType:
        case ScriptObject::IntegerType:
        case ScriptObject::FloatType:
        case ScriptObject::StringType:
            out.append(ConstantOp, scope->addConstant(expr));
            return;

        case ScriptObject::SymbolType: {
            Symbol* symbol = static_cast<Symbol*>(expr.data());
            ScriptObjectPointer binding = scope->resolve(symbol->name());
            if (binding.isNull()) {
                throw ScriptError("Unresolved symbol.", symbol->position());
            }
            switch (binding->type()) {
                case ScriptObject::ArgumentType: {
                    Argument* argument = static_cast<Argument*>(binding.data());
                    out.append(ArgumentOp, argument->index());
                    return;
                }
                case ScriptObject::MemberType: {
                    Member* member = static_cast<Member*>(binding.data());
                    out.append(MemberOp, member->scope(), member->index());
                    return;
                }
            }
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
                    ScriptObjectPointer member;
                    switch (cadr->type()) {
                        case ScriptObject::SymbolType: {
                            Symbol* variable = static_cast<Symbol*>(cadr.data());
                            member = scope->addMember(variable->name());
                            if (contents.size() == 2) {
                                return;

                            } else if (contents.size() == 3) {
                                compile(contents.at(2), scope,
                                    false, false, scope->initBytecode());

                            } else {
                                throw ScriptError("Invalid definition.", list->position());
                            }
                            break;
                        }
                        case ScriptObject::ListType: {
                            member = compileLambda(list, scope, scope->initBytecode(), true);
                            break;
                        }
                        default:
                            throw ScriptError("Invalid definition.", list->position());
                    }
                    Member* mem = static_cast<Member*>(member.data());
                    scope->initBytecode().append(SetMemberOp, 0, mem->index());
                    return;

                } else if (name == "lambda") {
                    compileLambda(list, scope, out);
                    return;

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
                    switch (binding->type()) {
                        case ScriptObject::ArgumentType: {
                            Argument* argument = static_cast<Argument*>(binding.data());
                            compile(contents.at(2), scope, false, false, out);
                            out.append(SetArgumentOp, argument->index());
                            out.append(ConstantOp, scope->addConstant(
                                ScriptObjectPointer(new Unspecified())));
                        }
                        case ScriptObject::MemberType: {
                            Member* member = static_cast<Member*>(binding.data());
                            compile(contents.at(2), scope, false, false, out);
                            out.append(SetMemberOp, member->scope(), member->index());
                            out.append(ConstantOp, scope->addConstant(
                                ScriptObjectPointer(new Unspecified())));
                        }
                    }
                    return;

                } else if (name == "quote") {
                    if (contents.size() != 2) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    out.append(ConstantOp, scope->addConstant(contents.at(1)));
                    return;
                    
                } else if (name == "if") {
                    int csize = contents.size();
                    if (csize != 3 && csize != 4) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    compile(contents.at(1), scope, false, false, out);
                    Bytecode thencode;
                    compile(contents.at(2), scope, false, tailCall, thencode);
                    Bytecode elsecode;
                    if (csize == 4) {
                        compile(contents.at(3), scope, false, tailCall, elsecode);
                    } else {
                        elsecode.append(ConstantOp, scope->addConstant(
                            ScriptObjectPointer(new Unspecified())));
                    }
                    thencode.append(JumpOp, elsecode.length());
                    out.append(ConditionalJumpOp, thencode.length());
                    out.append(thencode);
                    out.append(elsecode);
                    return;
                }
            }
            out.append(ResetOperandCountOp);
            foreach (const ScriptObjectPointer& operand, contents) {
                compile(operand, scope, false, false, out);
            }
            out.append(tailCall ? TailCallOp : CallOp);
            out.associate(list->position());
            return;
        }
    }
}

ScriptObjectPointer Evaluator::evaluate (const QString& expr)
{
    Parser parser(expr, _source);
    ScriptObjectPointer datum;
    ScriptObjectPointer result;
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(_scope.lambdaProc().data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    ScriptObjectPointerList expanded;
    Bytecode bodyBytecode;    
    while (!(datum = parser.parse()).isNull()) {
        expand(datum, &_scope, true, false, expanded);
        ensureDefsExpanded(&_scope, true, expanded);
        
        foreach (const ScriptObjectPointer& element, expanded) {
            compile(element, &_scope, true, false, bodyBytecode);
        
            if (!bodyBytecode.isEmpty()) {
                bodyBytecode.append(ExitOp);
                lambda->setConstantsAndBytecode(_scope.constants(), bodyBytecode);
                bodyBytecode.clear();
                
            } else {
                _scope.initBytecode().append(LambdaExitOp);
                lambda->setConstantsAndBytecode(_scope.constants(), _scope.initBytecode());
                _scope.initBytecode().clear();
            }
            _scope.constants().clear();
            
            _registers.instruction = lambda->body();
            result = execute();
            
            lambda->clearConstantsAndBytecode();
        }
        expanded.clear();
    }
    return result;
}

ScriptObjectPointer Evaluator::execute (int maxCycles)
{
    // initialize state
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(
        _stack.at(_registers.procedureIdx).data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());

    while (maxCycles == 0 || maxCycles-- > 0) {
        switch (*_registers.instruction++) {
            case ConstantOp:
                _stack.push(lambda->constant(qFromBigEndian<qint32>(_registers.instruction)));
                _registers.instruction += 4;
                _registers.operandCount++;
                break;

            case ArgumentOp:
                _stack.push(_stack.at(_registers.argumentIdx +
                    qFromBigEndian<qint32>(_registers.instruction)));
                _registers.instruction += 4;
                _registers.operandCount++;
                break;

            case MemberOp:
                _stack.push(proc->member(qFromBigEndian<qint32>(_registers.instruction),
                    qFromBigEndian<qint32>(_registers.instruction + 4)));
                _registers.instruction += 8;
                _registers.operandCount++;
                break;
            
            case SetArgumentOp:
                _stack[_registers.argumentIdx +
                    qFromBigEndian<qint32>(_registers.instruction)] = _stack.pop();
                _registers.instruction += 4;
                _registers.operandCount--;
                break;

            case SetMemberOp: 
                proc->setMember(qFromBigEndian<qint32>(_registers.instruction),
                    qFromBigEndian<qint32>(_registers.instruction + 4), _stack.pop());
                _registers.instruction += 8;
                _registers.operandCount--;
                break;
            
            case ResetOperandCountOp:
                _stack.push(ScriptObjectPointer(new Integer(_registers.operandCount)));
                _registers.operandCount = 0;
                break;

            case CallOp: {
                int pidx = _stack.size() - _registers.operandCount;
                ScriptObjectPointer sproc = _stack.at(pidx);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        int ocidx = pidx - 1;
                        ScriptObjectPointer* sdata = _stack.data();
                        ScriptObjectPointer result;
                        try {
                            result = nproc->function()(this, _registers.operandCount - 1,
                                sdata + ocidx + 2);
                        } catch (const QString& message) {
                            throwScriptError(message);
                            break;
                        }
                        _registers.operandCount = static_cast<Integer*>(
                            sdata[ocidx].data())->value() + 1;
                        _stack.resize(ocidx + 1);
                        _stack[ocidx] = result;
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        LambdaProcedure* nproc = static_cast<LambdaProcedure*>(sproc.data());
                        Lambda* nlambda = static_cast<Lambda*>(nproc->lambda().data());
                        if (nlambda->listArgument()) {
                            int lsize = _registers.operandCount - 1 -
                                nlambda->scalarArgumentCount();
                            if (lsize < 0) {
                                throwScriptError("Too few arguments.");
                                break;
                            }
                            QList<ScriptObjectPointer> contents;
                            int ssize = _stack.size() - lsize;
                            for (int ii = ssize, nn = _stack.size(); ii < nn; ii++) {
                                contents.append(_stack.at(ii));
                            }
                            _stack.resize(ssize);
                            _stack.push(ScriptObjectPointer(new List(contents)));
                            _registers.operandCount -= (lsize - 1);
                            
                        } else if (_registers.operandCount - 1 != nlambda->scalarArgumentCount()) {
                            throwScriptError("Wrong number of arguments.");
                            break;
                        }
                        _stack.push(ScriptObjectPointer(new Return(_registers)));
                        proc = nproc;
                        lambda = nlambda;
                        _registers.procedureIdx = pidx;
                        _registers.argumentIdx = pidx + 1;
                        _registers.instruction = lambda->body();
                        _registers.operandCount = 0;
                        break;
                    }
                    default:
                        throwScriptError("First operand not a procedure.");
                        break;
                }
                break;
            }
            case TailCallOp: {
                int pidx = _stack.size() - _registers.operandCount;
                ScriptObjectPointer sproc = _stack.at(pidx);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        int ocidx = pidx - 1;
                        ScriptObjectPointer* sdata = _stack.data();
                        ScriptObjectPointer result;
                        try {
                            result = nproc->function()(this, _registers.operandCount - 1,
                                sdata + ocidx + 2);
                        } catch (const QString& message) {
                            throwScriptError(message);
                            break;
                        }
                        _registers.operandCount = static_cast<Integer*>(
                            sdata[ocidx].data())->value() + 1;
                        _stack.resize(ocidx + 1);
                        _stack[ocidx] = result;
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        LambdaProcedure* nproc = static_cast<LambdaProcedure*>(sproc.data());
                        Lambda* nlambda = static_cast<Lambda*>(nproc->lambda().data());
                        if (nlambda->listArgument()) {
                            int lsize = _registers.operandCount - 1 -
                                nlambda->scalarArgumentCount();
                            if (lsize < 0) {
                                throwScriptError("Too few arguments.");
                                break;
                            }
                            QList<ScriptObjectPointer> contents;
                            int ssize = _stack.size() - lsize;
                            for (int ii = ssize, nn = _stack.size(); ii < nn; ii++) {
                                contents.append(_stack.at(ii));
                            }
                            _stack.resize(ssize);
                            _stack.push(ScriptObjectPointer(new List(contents)));
                            _registers.operandCount -= (lsize - 1);

                        } else if (_registers.operandCount - 1 != nlambda->scalarArgumentCount()) {
                            throwScriptError("Wrong number of arguments.");
                            break;
                        }
                        int pidx = _stack.size() - _registers.operandCount;
                        ScriptObjectPointer ret = _stack.at(pidx - 2);
                        _stack.remove(_registers.procedureIdx, pidx - _registers.procedureIdx);
                        _stack.push(ret);
                        proc = nproc;
                        lambda = nlambda;
                        _registers.instruction = lambda->body();
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
                int ocidx = _registers.procedureIdx - 1;
                ScriptObjectPointer& ocref = _stack[ocidx];
                Integer* oc = static_cast<Integer*>(ocref.data());
                int noc = oc->value() + 1;
                int ridx = _stack.size() - 1;
                ocref = _stack.at(ridx);
                Return* ret = static_cast<Return*>(_stack.at(ridx - 1).data());
                _registers = ret->registers();
                _registers.operandCount = noc;
                proc = static_cast<LambdaProcedure*>(_stack.at(_registers.procedureIdx).data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
                _stack.resize(ocidx + 1);
                break;
            }
            case LambdaOp: {
                ScriptObjectPointer nlambda = lambda->constant(
                    qFromBigEndian<qint32>(_registers.instruction));
                _registers.instruction += 4;
                lambda = static_cast<Lambda*>(nlambda.data());
                _stack.push(ScriptObjectPointer(proc = new LambdaProcedure(
                    nlambda, _stack.at(_registers.procedureIdx))));
                _registers.operandCount++;
                _stack.push(ScriptObjectPointer(new Return(_registers)));
                _registers.procedureIdx = _stack.size() - 2;
                _registers.instruction = lambda->initializer();
                _registers.operandCount = 0;
                break;
            }
            case LambdaReturnOp: {
                ScriptObjectPointer rptr = _stack.pop();
                Return* ret = static_cast<Return*>(rptr.data());
                _registers = ret->registers();
                proc = static_cast<LambdaProcedure*>(_stack.at(_registers.procedureIdx).data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
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
                return ScriptObjectPointer(new Unspecified());
            
            case JumpOp:
                _registers.instruction += qFromBigEndian<qint32>(_registers.instruction) + 4;
                break;
            
            case ConditionalJumpOp: {
                ScriptObjectPointer test = _stack.pop();
                if (test->type() == ScriptObject::BooleanType &&
                        static_cast<Boolean*>(test.data())->value()) {
                    _registers.instruction += 4;
                } else {
                    _registers.instruction += qFromBigEndian<qint32>(_registers.instruction) + 4;
                }
                break;
            }
        }
    }

    return ScriptObjectPointer();
}

void Evaluator::throwScriptError (const QString& message)
{
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(
        _stack.at(_registers.procedureIdx).data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    
    QVector<ScriptPosition> positions;
    positions.append(lambda->bytecode().position(_registers.instruction));
    
    while (_registers.procedureIdx != 0) {
        // remove the current set of operands; before it will either be a return frame or an oc
        int fidx = _stack.size() - _registers.operandCount - 1;
        ScriptObjectPointer frame = _stack.at(fidx);
        _stack.resize(fidx);
        switch (frame->type()) {
            case ScriptObject::IntegerType: {
                Integer* oc = static_cast<Integer*>(frame.data());
                _registers.operandCount = oc->value();
                break;
            }
            case ScriptObject::ReturnType: {
                Return* ret = static_cast<Return*>(frame.data());
                _registers = ret->registers();
                proc = static_cast<LambdaProcedure*>(_stack.at(_registers.procedureIdx).data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
                positions.append(lambda->bytecode().position(_registers.instruction));
                break;
            }
        }
    }
    
    // remove any remaining operands
    if (_registers.operandCount != 0) {
        _stack.resize(1);
        _registers.operandCount = 0;
    }
    
    throw ScriptError(message, positions);
}
