//
// $Id$

#include <QtDebug>

#include "script/Evaluator.h"
#include "script/Globals.h"

/**
 * Adds numbers together, returns the result.
 */
static ScriptObjectPointer add (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    int isum = 0;
    for (ScriptObjectPointer* arg = argv, *end = arg + argc; arg != end; arg++) {
        switch ((*arg)->type()) {
            case ScriptObject::IntegerType:
                isum += static_cast<Integer*>(arg->data())->value();
                break;
                
            case ScriptObject::FloatType: { // promote to float
                float fsum = isum + static_cast<Float*>(arg->data())->value();
                for (arg++; arg != end; arg++) {
                    switch ((*arg)->type()) {
                        case ScriptObject::IntegerType:
                            fsum += static_cast<Integer*>(arg->data())->value();
                            break;
                            
                        case ScriptObject::FloatType:
                            fsum += static_cast<Float*>(arg->data())->value();
                            break;
                            
                        default:
                            throw QString("Invalid argument.");
                    }
                }
                return ScriptObjectPointer(new Float(fsum));
            }
            default:
                throw QString("Invalid argument.");
        }
    }
    return Integer::instance(isum);
}

/**
 * Negates a number or subtracts numbers, returns the result.
 */
static ScriptObjectPointer subtract (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int isum = static_cast<Integer*>(argv->data())->value();
            if (argc == 1) {
                return Integer::instance(-isum);
            }
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        isum -= static_cast<Integer*>(arg->data())->value();
                        break;
                        
                    case ScriptObject::FloatType: { // promote to float
                        float fsum = isum - static_cast<Float*>(arg->data())->value();
                        for (arg++; arg != end; arg++) {
                            switch ((*arg)->type()) {
                                case ScriptObject::IntegerType:
                                    fsum -= static_cast<Integer*>(arg->data())->value();
                                    break;
                                    
                                case ScriptObject::FloatType:
                                    fsum -= static_cast<Float*>(arg->data())->value();
                                    break;
                                    
                                default:
                                    throw QString("Invalid argument.");
                            }
                        }
                        return ScriptObjectPointer(new Float(fsum));
                    }
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return Integer::instance(isum);
        }
        case ScriptObject::FloatType: {
            float fsum = static_cast<Float*>(argv->data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(-fsum));
            }
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        fsum -= static_cast<Integer*>(arg->data())->value();
                        break;
                        
                    case ScriptObject::FloatType:
                        fsum -= static_cast<Float*>(arg->data())->value();
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return ScriptObjectPointer(new Float(fsum)); 
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Multiplies numbers together, returns the result.
 */
static ScriptObjectPointer multiply (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    int iprod = 1;
    for (ScriptObjectPointer* arg = argv, *end = arg + argc; arg != end; arg++) {
        switch ((*arg)->type()) {
            case ScriptObject::IntegerType:
                iprod *= static_cast<Integer*>(arg->data())->value();
                break;
                
            case ScriptObject::FloatType: { // promote to float
                float fprod = iprod * static_cast<Float*>(arg->data())->value();
                for (arg++; arg != end; arg++) {
                    switch ((*arg)->type()) {
                        case ScriptObject::IntegerType:
                            fprod *= static_cast<Integer*>(arg->data())->value();
                            break;
                            
                        case ScriptObject::FloatType:
                            fprod *= static_cast<Float*>(arg->data())->value();
                            break;
                            
                        default:
                            throw QString("Invalid argument.");
                    }
                }
                return ScriptObjectPointer(new Float(fprod));
            }
            default:
                throw QString("Invalid argument.");
        }
    }
    return Integer::instance(iprod);
}

/**
 * Inverts a number or divides numbers, returns the result.
 */
static ScriptObjectPointer divide (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int iprod = static_cast<Integer*>(argv->data())->value();
            if (iprod == 0) {
                throw QString("Division by zero.");
            }
            if (argc == 1) {
                return Integer::instance(1 / iprod);
            }
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType: {
                        int value = static_cast<Integer*>(arg->data())->value();
                        if (value == 0) {
                            throw QString("Division by zero.");
                        }
                        iprod /= value;
                        break;
                    }
                    case ScriptObject::FloatType: { // promote to float
                        float fprod = iprod / static_cast<Float*>(arg->data())->value();
                        for (arg++; arg != end; arg++) {
                            switch ((*arg)->type()) {
                                case ScriptObject::IntegerType:
                                    fprod /= static_cast<Integer*>(arg->data())->value();
                                    break;
                                    
                                case ScriptObject::FloatType:
                                    fprod /= static_cast<Float*>(arg->data())->value();
                                    break;
                                    
                                default:
                                    throw QString("Invalid argument.");
                            }
                        }
                        return ScriptObjectPointer(new Float(fprod));
                    }
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return Integer::instance(iprod);
        }
        case ScriptObject::FloatType: {
            float fprod = static_cast<Float*>(argv->data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(1.0 / fprod));
            }
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        fprod /= static_cast<Integer*>(arg->data())->value();
                        break;
                        
                    case ScriptObject::FloatType:
                        fprod /= static_cast<Float*>(arg->data())->value();
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return ScriptObjectPointer(new Float(fprod));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns whether all arguments are equal.
 */
static ScriptObjectPointer equal (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return Boolean::instance(true);
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int ival = static_cast<Integer*>(argv->data())->value();
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        if (static_cast<Integer*>(arg->data())->value() != ival) {
                            return Boolean::instance(false);
                        }
                        break;
                        
                    case ScriptObject::FloatType:
                        if (static_cast<Float*>(arg->data())->value() != ival) {
                            return Boolean::instance(false);
                        }
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return Boolean::instance(true);
        }
        case ScriptObject::FloatType: {
            float fval = static_cast<Integer*>(argv->data())->value();
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        if (static_cast<Integer*>(arg->data())->value() != fval) {
                            return Boolean::instance(false);
                        }
                        break;
                        
                    case ScriptObject::FloatType:
                        if (static_cast<Float*>(arg->data())->value() != fval) {
                            return Boolean::instance(false);
                        }
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return Boolean::instance(true);
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns whether all arguments are monotonically increasing.
 */
static ScriptObjectPointer less (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return Boolean::instance(true);
    }
    return Boolean::instance(false);
}

/**
 * Returns whether all arguments are monotonically decreasing.
 */
static ScriptObjectPointer greater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return Boolean::instance(true);
    }
    return Boolean::instance(false);
}

/**
 * Returns whether all arguments are monotonically nondecreasing.
 */
static ScriptObjectPointer lessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return Boolean::instance(true);
    }
    return Boolean::instance(false);
}

/**
 * Returns whether all arguments are monotonically nonincreasing.
 */
static ScriptObjectPointer greaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return Boolean::instance(true);
    }
    return Boolean::instance(false);
}

/**
 * Checks whether the argument is zero.
 */
static ScriptObjectPointer zero (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return Boolean::instance(static_cast<Integer*>(argv->data())->value() == 0);
            
        case ScriptObject::FloatType:
            return Boolean::instance(static_cast<Float*>(argv->data())->value() == 0);
            
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns #t if argument is false, otherwise returns #f.
 */
static ScriptObjectPointer notfunc (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    bool value = true;
    if ((*argv)->type() == ScriptObject::BooleanType) {
        value = static_cast<Boolean*>(argv->data())->value();
    }
    return Boolean::instance(!value);
}

/**
 * Creates a new list from the arguments.
 */
static ScriptObjectPointer list (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    ScriptObjectPointerList contents;
    for (ScriptObjectPointer* arg = argv, *end = arg + argc; arg != end; arg++) {
        contents.append(*arg);
    }
    return List::instance(contents);
}

/**
 * Determines whether the argument is a boolean.
 */
static ScriptObjectPointer boolean (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::BooleanType);
}

/**
 * Determines whether the argument is a symbol.
 */
static ScriptObjectPointer symbol (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::SymbolType);
}

/**
 * Determines whether the argument is a number.
 */
static ScriptObjectPointer number (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    ScriptObject::Type type = (*argv)->type();
    return Boolean::instance(type == ScriptObject::IntegerType || type == ScriptObject::FloatType);
}

/**
 * Determines whether the argument is a string.
 */
static ScriptObjectPointer string (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::StringType);
}

/**
 * Determines whether the argument is a procedure.
 */
static ScriptObjectPointer procedure (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    ScriptObject::Type type = (*argv)->type();
    return Boolean::instance(type == ScriptObject::NativeProcedureType ||
        type == ScriptObject::LambdaProcedureType ||
        type == ScriptObject::CaptureProcedureType ||
        type == ScriptObject::EscapeProcedureType);
}

/**
 * Creates the global scope object.
 */
static Scope createGlobalScope ()
{
    Scope scope(0, true);
    
    scope.addVariable("+", add);
    scope.addVariable("-", subtract);
    scope.addVariable("*", multiply);
    scope.addVariable("/", divide);
    
    scope.addVariable("=", equal);
    scope.addVariable("<", less);
    scope.addVariable(">", greater);
    scope.addVariable("<=", lessEqual);
    scope.addVariable(">=", greaterEqual);
    
    scope.addVariable("zero?", zero);
    
    scope.addVariable("not", notfunc);
    
    scope.addVariable("list", list);
    
    scope.addVariable("boolean?", boolean);
    scope.addVariable("symbol?", symbol);
    scope.addVariable("number?", number);
    scope.addVariable("string?", string);
    scope.addVariable("procedure?", procedure);
    
    ScriptObjectPointer callcc(new CaptureProcedure());
    scope.addVariable("call-with-current-continuation", ScriptObjectPointer(), callcc);
    scope.addVariable("call/cc", ScriptObjectPointer(), callcc);
    
    return scope;
}

Scope* globalScope ()
{
    static Scope global = createGlobalScope();
    return &global;
}
