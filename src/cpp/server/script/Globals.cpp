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
 * Creates the global scope object.
 */
static Scope createGlobalScope ()
{
    Scope scope(0, true);
    scope.define("+", add);
    scope.define("*", multiply);
    return scope;
}

Scope* globalScope ()
{
    static Scope global = createGlobalScope();
    return &global;
}
