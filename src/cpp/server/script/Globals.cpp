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
 * Compares booleans for equality.
 */
static ScriptObjectPointer booleansEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::BooleanType) {
        throw QString("Invalid argument.");
    }
    Boolean* boolean = static_cast<Boolean*>(argv->data());
    bool value = boolean->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::BooleanType) {
            throw QString("Invalid argument.");
        }
        boolean = static_cast<Boolean*>(arg->data());
        if (boolean->value() != value) {
            return Boolean::instance(false);
        }
    }
    return Boolean::instance(true);
}

/**
 * Converts a symbol to a string.
 */
static ScriptObjectPointer symbolToString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::SymbolType) {
        throw QString("Invalid argument.");
    }
    Symbol* symbol = static_cast<Symbol*>(argv->data());
    return ScriptObjectPointer(new String(symbol->name()));
}

/**
 * Converts a string to a symbol.
 */
static ScriptObjectPointer stringToSymbol (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    String* string = static_cast<String*>(argv->data());
    return Symbol::instance(string->contents());
}

/**
 * Compares symbols for equality.
 */
static ScriptObjectPointer symbolsEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::SymbolType) {
        throw QString("Invalid argument.");
    }
    Symbol* symbol = static_cast<Symbol*>(argv->data());
    const QString& name = symbol->name();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::SymbolType) {
            throw QString("Invalid argument.");
        }
        symbol = static_cast<Symbol*>(arg->data());
        if (symbol->name() != name) {
            return Boolean::instance(false);
        }
    }
    return Boolean::instance(true);
}

/**
 * Checks whether the argument is the empty list.
 */
static ScriptObjectPointer null (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::ListType) {
        return Boolean::instance(false);
    }
    List* list = static_cast<List*>(argv->data());
    return Boolean::instance(list->contents().isEmpty());
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
    return eval->listInstance(contents);
}

/**
 * Appends the arguments together.
 */
static ScriptObjectPointer append (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        return List::instance(ScriptObjectPointerList());
    }
    if (argc == 1) {
        if ((*argv)->type() != ScriptObject::ListType) {
            throw QString("Invalid argument.");
        }
        return *argv;
    }
    ScriptObjectPointerList contents;
    for (ScriptObjectPointer* arg = argv, *end = arg + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::ListType) {
            throw QString("Invalid argument.");
        }
        List* list = static_cast<List*>(arg->data());
        contents.append(list->contents());
    } 
    return eval->listInstance(contents);
}

/**
 * Returns the length of a list.
 */
static ScriptObjectPointer length (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::ListType) {
        throw QString("Invalid argument.");
    }
    List* list = static_cast<List*>(argv->data());
    return Integer::instance(list->contents().length());
}

/**
 * Reverses a list.
 */
static ScriptObjectPointer reverse (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::ListType) {
        throw QString("Invalid argument.");
    }
    List* list = static_cast<List*>(argv->data());
    const ScriptObjectPointerList& contents = list->contents();
    ScriptObjectPointerList ncontents;
    for (int ii = contents.size() - 1; ii >= 0; ii--) {
        ncontents.append(contents.at(ii));
    }
    return eval->listInstance(ncontents);
}

/**
 * Determines whether the argument is a list.
 */
static ScriptObjectPointer listp (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::ListType);
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
    scope.addVariable("boolean=?", booleansEqual);
    
    scope.addVariable("symbol->string", symbolToString);
    scope.addVariable("string->symbol", stringToSymbol);
    scope.addVariable("symbol=?", symbolsEqual);
    
    scope.addVariable("null?", null);
    scope.addVariable("list", ScriptObjectPointer(), listProcedure());
    scope.addVariable("append", ScriptObjectPointer(), appendProcedure());
    scope.addVariable("length", length);
    scope.addVariable("reverse", reverse);
    
    scope.addVariable("list?", listp);
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

const ScriptObjectPointer& listProcedure ()
{
    static ScriptObjectPointer proc(new NativeProcedure(list));
    return proc;
}

const ScriptObjectPointer& appendProcedure ()
{
    static ScriptObjectPointer proc(new NativeProcedure(append));
    return proc;
}
