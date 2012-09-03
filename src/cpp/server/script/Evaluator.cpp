//
// $Id$

#include <QStringList>
#include <QtDebug>
#include <QtEndian>

#include "script/Evaluator.h"
#include "script/Globals.h"
#include "script/Parser.h"

Scope::Scope (Scope* parent, bool withValues) :
    _parent(parent),
    _memberCount(0),
    _lambdaProc(withValues ? new LambdaProcedure(ScriptObjectPointer(new Lambda()),
        parent == 0 ? ScriptObjectPointer() : parent->_lambdaProc) : 0)
{
}

void Scope::setArguments (const QStringList& firstArgs, const QString& restArg)
{
    int idx = 0;
    foreach (const QString& arg, firstArgs) {
        _variables.insert(arg, ScriptObjectPointer(new Argument(idx++)));
    }
    if (!restArg.isEmpty()) {
        _variables.insert(restArg, ScriptObjectPointer(new Argument(idx++)));
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
    if (_parent != 0) {
        ScriptObjectPointer pbinding = _parent->resolve(name);
        if (!pbinding.isNull()) {
            switch (pbinding->type()) {
                case ScriptObject::ArgumentType: {
                    // copy parent argument to local member on initialization
                    Argument* argument = static_cast<Argument*>(pbinding.data());
                    _initBytecode.append(ArgumentOp, argument->index());
                    _initBytecode.append(SetMemberOp, 0, _memberCount);
                    return define(name);
                }
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

ScriptObjectPointer Scope::define (const QString& name, NativeProcedure::Function function)
{
    return define(name, ScriptObjectPointer(new NativeProcedure(function)));
}

ScriptObjectPointer Scope::define (const QString& name, const ScriptObjectPointer& value)
{
    int idx = _memberCount++;
    ScriptObjectPointer member(new Member(0, idx));
    _variables.insert(name, member);
    if (!_lambdaProc.isNull()) {
        LambdaProcedure* proc = static_cast<LambdaProcedure*>(_lambdaProc.data());
        proc->appendMember(value);
    }
    return member;
}

int Scope::addConstant (ScriptObjectPointer value)
{
    _constants.append(value);
    return _constants.size() - 1;
}

Evaluator::Evaluator (const QString& source) :
    _source(source),
    _scope(globalScope(), true),
    _procedureIdx(0),
    _argumentIdx(1),
    _operandCount(0)
{
    _stack.push(_scope.lambdaProc());
}

ScriptObjectPointer Evaluator::evaluate (const QString& expr)
{
    Parser parser(expr, _source);
    ScriptObjectPointer datum;
    ScriptObjectPointer result;
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(_scope.lambdaProc().data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    Bytecode bodyBytecode;
    while (!(datum = parser.parse()).isNull()) {
        compile(datum, &_scope, true, bodyBytecode);
        
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
        
        _instruction = lambda->body();
        result = execute();
        
        lambda->clearConstantsAndBytecode();
    }
    return result;
}

ScriptObjectPointer Evaluator::execute (int maxCycles)
{
    // copy members to locals
    int procedureIdx = _procedureIdx;
    int argumentIdx = _argumentIdx;
    const uchar* instruction = _instruction;
    int operandCount = _operandCount;
    QStack<ScriptObjectPointer>& stack = _stack;

    // initialize state
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(stack.at(procedureIdx).data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());

    while (maxCycles == 0 || maxCycles-- > 0) {
        switch (*instruction++) {
            case ConstantOp:
                stack.push(lambda->constant(qFromBigEndian<qint32>(instruction)));
                instruction += 4;
                operandCount++;
                break;

            case ArgumentOp:
                stack.push(stack.at(argumentIdx + qFromBigEndian<qint32>(instruction)));
                instruction += 4;
                operandCount++;
                break;

            case MemberOp:
                stack.push(proc->member(qFromBigEndian<qint32>(instruction),
                    qFromBigEndian<qint32>(instruction + 4)));
                instruction += 8;
                operandCount++;
                break;
            
            case SetArgumentOp:
                stack[argumentIdx + qFromBigEndian<qint32>(instruction)] = stack.pop();
                instruction += 4;
                operandCount--;
                break;

            case SetMemberOp: 
                proc->setMember(qFromBigEndian<qint32>(instruction),
                    qFromBigEndian<qint32>(instruction + 4), stack.pop());
                instruction += 8;
                operandCount--;
                break;
            
            case ResetOperandCountOp:
                stack.push(ScriptObjectPointer(new Integer(operandCount)));
                operandCount = 0;
                break;

            case CallOp: {
                int pidx = stack.size() - operandCount;
                ScriptObjectPointer sproc = stack.at(pidx);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        int ocidx = pidx - 1;
                        ScriptObjectPointer* sdata = stack.data();
                        ScriptObjectPointer result;
                        try {
                            result = nproc->function()(this, operandCount - 1, sdata + ocidx + 2);
                        } catch (const QString& message) {
                            throw ScriptError(message,
                                lambda->bytecode().position(instruction - 1));
                        }
                        operandCount = static_cast<Integer*>(sdata[ocidx].data())->value() + 1;
                        stack.resize(ocidx + 1);
                        stack[ocidx] = result;
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        LambdaProcedure* nproc = static_cast<LambdaProcedure*>(sproc.data());
                        Lambda* nlambda = static_cast<Lambda*>(nproc->lambda().data());
                        if (nlambda->listArgument()) {
                            int lsize = operandCount - 1 - nlambda->scalarArgumentCount();
                            if (lsize < 0) {
                                throw ScriptError("Too few arguments.",
                                    lambda->bytecode().position(instruction - 1));
                            }
                            QList<ScriptObjectPointer> contents;
                            int ssize = stack.size() - lsize;
                            for (int ii = ssize, nn = stack.size(); ii < nn; ii++) {
                                contents.append(stack.at(ii));
                            }
                            stack.resize(ssize);
                            stack.push(ScriptObjectPointer(new List(contents)));

                        } else if (operandCount - 1 != nlambda->scalarArgumentCount()) {
                            throw ScriptError("Wrong number of arguments.",
                                lambda->bytecode().position(instruction - 1));
                        }
                        stack.push(ScriptObjectPointer(new Return(
                            procedureIdx, argumentIdx, instruction, operandCount)));
                        proc = nproc;
                        lambda = nlambda;
                        procedureIdx = pidx;
                        argumentIdx = pidx + 1;
                        instruction = lambda->body();
                        operandCount = 0;
                        break;
                    }
                    default:
                        throw ScriptError("First operand not a procedure.",
                            lambda->bytecode().position(instruction - 1));
                }
                break;
            }
            case ReturnOp: {
                int ocidx = procedureIdx - 1;
                ScriptObjectPointer& ocref = stack[ocidx];
                Integer* oc = static_cast<Integer*>(ocref.data());
                operandCount = oc->value() + 1;
                int ridx = stack.size() - 1;
                ocref = stack.at(ridx);
                Return* ret = static_cast<Return*>(stack.at(ridx - 1).data());
                procedureIdx = ret->procedureIdx();
                argumentIdx = ret->argumentIdx();
                instruction = ret->instruction();
                proc = static_cast<LambdaProcedure*>(stack.at(procedureIdx).data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
                stack.resize(ocidx + 1);
                break;
            }
            case LambdaOp: {
                ScriptObjectPointer nlambda = lambda->constant(
                    qFromBigEndian<qint32>(instruction));
                instruction += 4;
                lambda = static_cast<Lambda*>(nlambda.data());
                stack.push(ScriptObjectPointer(proc = new LambdaProcedure(
                    nlambda, stack.at(procedureIdx))));
                operandCount++;
                stack.push(ScriptObjectPointer(new Return(
                    procedureIdx, argumentIdx, instruction, operandCount)));
                procedureIdx = stack.size() - 2;
                instruction = lambda->initializer();
                operandCount = 0;
                break;
            }
            case LambdaReturnOp: {
                ScriptObjectPointer rptr = stack.pop();
                Return* ret = static_cast<Return*>(rptr.data());
                procedureIdx = ret->procedureIdx();
                instruction = ret->instruction();
                operandCount = ret->operandCount();
                proc = static_cast<LambdaProcedure*>(stack.at(procedureIdx).data());
                lambda = static_cast<Lambda*>(proc->lambda().data());
                break;
            }
            case PopOp:
                stack.pop();
                operandCount--;
                break;
                
            case ExitOp:
                return stack.pop();
                
            case LambdaExitOp:
                return ScriptObjectPointer(new Unspecified());
        }
    }

    // restore members
    _procedureIdx = procedureIdx;
    _argumentIdx = argumentIdx;
    _instruction = instruction;
    _operandCount = operandCount;

    return ScriptObjectPointer();
}

void Evaluator::compile (ScriptObjectPointer expr, Scope* scope, bool allowDef, Bytecode& out)
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
                            member = scope->define(variable->name());
                            if (contents.size() == 2) {
                                return;

                            } else if (contents.size() == 3) {
                                compile(contents.at(2), scope, false, scope->initBytecode());

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
                    compile(contents.at(2), scope, false, out);
                    switch (binding->type()) {
                        case ScriptObject::ArgumentType: {
                            Argument* argument = static_cast<Argument*>(binding.data());
                            out.append(SetArgumentOp, argument->index());
                        }
                        case ScriptObject::MemberType: {
                            Member* member = static_cast<Member*>(binding.data());
                            out.append(SetMemberOp, member->scope(), member->index());
                        }
                    }
                    out.append(ConstantOp, scope->addConstant(ScriptObjectPointer(new Unspecified())));
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
                    compile(contents.at(1), scope, false, out);
                    
                }
            }
            out.append(ResetOperandCountOp);
            foreach (const ScriptObjectPointer& operand, contents) {
                compile(operand, scope, false, out);
            }
            out.append(CallOp, list->position());
            return;
        }
    }
}

ScriptObjectPointer Evaluator::compileLambda (
    List* list, Scope* scope, Bytecode& out, bool define)
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
                    member = scope->define(var->name());
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
                            compile(element, &subscope, true, bodyBytecode);
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
        compile(element, &subscope, false, bodyBytecode);
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
    return member;
}

