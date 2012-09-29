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
    return ScriptObjectPointer(new Integer(isum));
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
                return ScriptObjectPointer(new Integer(-isum));
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
            return ScriptObjectPointer(new Integer(isum));
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
    return ScriptObjectPointer(new Integer(iprod));
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
                return ScriptObjectPointer(new Integer(1 / iprod));
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
            return ScriptObjectPointer(new Integer(iprod));
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
        return ScriptObjectPointer(new Boolean(true));
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int ival = static_cast<Integer*>(argv->data())->value();
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        if (static_cast<Integer*>(arg->data())->value() != ival) {
                            return ScriptObjectPointer(new Boolean(false));
                        }
                        break;
                        
                    case ScriptObject::FloatType:
                        if (static_cast<Float*>(arg->data())->value() != ival) {
                            return ScriptObjectPointer(new Boolean(false));
                        }
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return ScriptObjectPointer(new Boolean(true));
        }
        case ScriptObject::FloatType: {
            float fval = static_cast<Integer*>(argv->data())->value();
            for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
                switch ((*arg)->type()) {
                    case ScriptObject::IntegerType:
                        if (static_cast<Integer*>(arg->data())->value() != fval) {
                            return ScriptObjectPointer(new Boolean(false));
                        }
                        break;
                        
                    case ScriptObject::FloatType:
                        if (static_cast<Float*>(arg->data())->value() != fval) {
                            return ScriptObjectPointer(new Boolean(false));
                        }
                        break;
                        
                    default:
                        throw QString("Invalid argument.");
                }
            }
            return ScriptObjectPointer(new Boolean(true));
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
        return ScriptObjectPointer(new Boolean(true));
    }
    return ScriptObjectPointer(new Boolean(false));
}

/**
 * Returns whether all arguments are monotonically decreasing.
 */
static ScriptObjectPointer greater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return ScriptObjectPointer(new Boolean(true));
    }
    return ScriptObjectPointer(new Boolean(false));
}

/**
 * Returns whether all arguments are monotonically nondecreasing.
 */
static ScriptObjectPointer lessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return ScriptObjectPointer(new Boolean(true));
    }
    return ScriptObjectPointer(new Boolean(false));
}

/**
 * Returns whether all arguments are monotonically nonincreasing.
 */
static ScriptObjectPointer greaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return ScriptObjectPointer(new Boolean(true));
    }
    return ScriptObjectPointer(new Boolean(false));
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
    return ScriptObjectPointer(new Boolean(!value));
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
    return ScriptObjectPointer(new List(contents));
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
    
    scope.addVariable("not", notfunc);
    
    scope.addVariable("list", list);
    
    return scope;
}

Scope* globalScope ()
{
    static Scope global = createGlobalScope();
    return &global;
}
