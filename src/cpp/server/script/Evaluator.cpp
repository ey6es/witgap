//
// $Id$

#include <QStringList>

#include "script/Evaluator.h"
#include "script/Parser.h"

/** The bytecode instructions. */
enum {

    /** Pushes the constant at the specified index onto the stack. */
    ConstantOp,

    /** Pushes the argument at the specified index onto the stack. */
    ArgumentOp,

    /** Pushes the member at the specified scope/index onto the stack. */
    MemberOp,

    /** Pops a value off the stack and assigns it to the argument at the specified index. */
    SetArgumentOp,

    /** Pops a value off the stack and assigns it to the member at the specified scope/index. */
    SetMemberOp,

    /** Pushes the current operand count and sets the count to zero. */
    ResetOperandCountOp,

    /** Performs a function call. */
    CallOp,

    /** Returns from a function call. */
    ReturnOp,

    /** Pops a value off the stack and discards it. */
    PopOp
};

/**
 * Writes an integer in network byte order to the specified ByteArray.
 */
inline void writeInteger (QByteArray& array, int value)
{
    array.append(value >> 24);
    array.append(value >> 16);
    array.append(value >> 8);
    array.append(value);
}

/**
 * Reads and returns an integer in network byte order, advancing the given pointer.
 */
inline int readInteger (const unsigned char*& bytecode)
{
    return (*bytecode++) << 24 |
        (*bytecode++) << 16 |
        (*bytecode++) << 8 |
        (*bytecode++);
}

Scope::Scope (Scope* parent) :
    _parent(parent),
    _memberCount(0)
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
                    _initBytecode.append(ArgumentOp);
                    writeInteger(_initBytecode, argument->index());
                    _initBytecode.append(SetMemberOp);
                    writeInteger(_initBytecode, 0);
                    writeInteger(_initBytecode, _memberCount);
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

ScriptObjectPointer Scope::define (const QString& name)
{
    ScriptObjectPointer member(new Member(0, _memberCount++));
    _variables.insert(name, member);
    return member;
}

int Scope::addConstant (ScriptObjectPointer value)
{
    _constants.append(value);
    return _constants.size() - 1;
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
        QByteArray out;
        compile(datum, &_scope, true, out);

    }
    return result;
}

ScriptObjectPointer Evaluator::execute (int maxCycles)
{
    // copy members to locals
    int procedureIdx = _procedureIdx;
    int argumentIdx = _argumentIdx;
    int instructionIdx = _instructionIdx;
    int operandCount = _operandCount;
    QStack<ScriptObjectPointer>& stack = _stack;

    // initialize state
    LambdaProcedure* proc = static_cast<LambdaProcedure*>(stack.at(procedureIdx).data());
    Lambda* lambda = static_cast<Lambda*>(proc->lambda().data());
    const unsigned char* bytecode = (const unsigned char*)lambda->bytecode().constData() +
        instructionIdx;

    while (maxCycles == 0 || maxCycles-- > 0) {
        switch (*bytecode++) {
            case ConstantOp:
                stack.push(lambda->constant(readInteger(bytecode)));
                operandCount++;
                break;

            case ArgumentOp:
                stack.push(stack.at(argumentIdx + readInteger(bytecode)));
                operandCount++;
                break;

            case MemberOp:
                stack.push(proc->member(readInteger(bytecode), readInteger(bytecode)));
                operandCount++;
                break;

            case SetArgumentOp:
                stack[argumentIdx + readInteger(bytecode)] = stack.pop();
                operandCount--;
                break;

            case SetMemberOp:
                proc->setMember(readInteger(bytecode), readInteger(bytecode), stack.pop());
                operandCount--;
                break;

            case ResetOperandCountOp:
                stack.push(ScriptObjectPointer(new Integer(operandCount)));
                operandCount = 0;
                break;

            case CallOp: {
                ScriptObjectPointer sproc = stack.at(stack.size() - operandCount);
                switch (sproc->type()) {
                    case ScriptObject::NativeProcedureType: {
                        NativeProcedure* nproc = static_cast<NativeProcedure*>(sproc.data());
                        nproc->call(stack, operandCount);
                        operandCount = 1;
                        break;
                    }
                    case ScriptObject::LambdaProcedureType: {
                        LambdaProcedure* lproc = static_cast<LambdaProcedure*>(sproc.data());
                        stack.push(ScriptObjectPointer(new Return(
                            procedureIdx, argumentIdx, instructionIdx, operandCount)));

                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case ReturnOp:
                break;

            case PopOp:
                stack.pop();
                operandCount--;
                break;
        }
    }

    // restore members
    _procedureIdx = procedureIdx;
    _argumentIdx = argumentIdx;
    _instructionIdx = instructionIdx;
    _operandCount = operandCount;

    return ScriptObjectPointer();
}

void Evaluator::compile (ScriptObjectPointer expr, Scope* scope, bool allowDef, QByteArray& out)
{
    switch (expr->type()) {
        case ScriptObject::BooleanType:
        case ScriptObject::StringType:
            out.append(ConstantOp);
            writeInteger(out, scope->addConstant(expr));
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
                    out.append(ArgumentOp);
                    writeInteger(out, argument->index());
                    return;
                }
                case ScriptObject::MemberType: {
                    Member* member = static_cast<Member*>(binding.data());
                    out.append(MemberOp);
                    writeInteger(out, member->scope());
                    writeInteger(out, member->index());
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
                    scope->initBytecode().append(SetMemberOp);
                    writeInteger(scope->initBytecode(), 0);
                    writeInteger(scope->initBytecode(), mem->index());
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
                            out.append(SetArgumentOp);
                            writeInteger(out, argument->index());
                        }
                        case ScriptObject::MemberType: {
                            Member* member = static_cast<Member*>(binding.data());
                            out.append(SetMemberOp);
                            writeInteger(out, member->scope());
                            writeInteger(out, member->index());
                        }
                    }
                    out.append(ConstantOp);
                    writeInteger(out, scope->addConstant(ScriptObjectPointer(new Unspecified())));
                    return;

                } else if (name == "quote") {
                    if (contents.size() != 2) {
                        throw ScriptError("Invalid expression.", list->position());
                    }
                    out.append(ConstantOp);
                    writeInteger(out, scope->addConstant(contents.at(1)));
                    return;
                }
            }
            out.append(ResetOperandCountOp);
            foreach (const ScriptObjectPointer& operand, contents) {
                compile(operand, scope, false, out);
            }
            out.append(CallOp);
            return;
        }
    }
}

ScriptObjectPointer Evaluator::compileLambda (
    List* list, Scope* scope, QByteArray& out, bool define)
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
    QByteArray bodyBytecode;
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
    bodyBytecode.append(ReturnOp);
    ScriptObjectPointer lambda(new Lambda(firstArgs.size(), !restArg.isEmpty(),
        subscope.memberCount(), subscope.constants(), subscope.initBytecode() + bodyBytecode,
        subscope.initBytecode().length()));
    out.append(ConstantOp);
    writeInteger(out, scope->addConstant(lambda));
    return member;
}
