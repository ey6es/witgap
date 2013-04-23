//
// $Id$

#include <cmath>

#include <QCoreApplication>
#include <QMutex>
#include <QWaitCondition>
#include <QtDebug>

#include "script/Evaluator.h"
#include "script/Globals.h"

/**
 * Adds numbers together, returns the result.
 */
static ScriptObjectPointer eqv (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    ScriptObject::Type t1 = argv[0]->type();
    if (t1 != argv[1]->type()) {
        return Boolean::instance(false);
    }
    switch (t1) {
        case ScriptObject::BooleanType:
            return Boolean::instance(static_cast<Boolean*>(argv[0].data())->value() ==
                static_cast<Boolean*>(argv[1].data())->value());

        case ScriptObject::SymbolType:
            return Boolean::instance(static_cast<Symbol*>(argv[0].data())->name() ==
                static_cast<Symbol*>(argv[1].data())->name());

        case ScriptObject::CharType:
            return Boolean::instance(static_cast<Char*>(argv[0].data())->value() ==
                static_cast<Char*>(argv[1].data())->value());

        case ScriptObject::IntegerType:
            return Boolean::instance(static_cast<Integer*>(argv[0].data())->value() ==
                static_cast<Integer*>(argv[1].data())->value());

        case ScriptObject::FloatType:
            return Boolean::instance(static_cast<Float*>(argv[0].data())->value() ==
                static_cast<Float*>(argv[1].data())->value());

        case ScriptObject::NullType:
            return Boolean::instance(true);

        case ScriptObject::WrappedObjectType:
            return Boolean::instance(static_cast<WrappedObject*>(argv[0].data())->object() ==
                static_cast<WrappedObject*>(argv[1].data())->object());

        default:
            return Boolean::instance(argv[0].data() == argv[1].data());
    }
}

/**
 * Checks whether the arguments are equal.
 */
static ScriptObjectPointer equal (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    return Boolean::instance(equivalent(argv[0], argv[1]));
}

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
 * Returns the remainder of the arguments.
 */
static ScriptObjectPointer mod (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    switch (argv[0]->type()) {
        case ScriptObject::IntegerType: {
            int v0 = static_cast<Integer*>(argv[0].data())->value();
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return Integer::instance(v0 % v1);
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(fmod(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        case ScriptObject::FloatType: {
            float v0 = static_cast<Float*>(argv[0].data())->value();
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(fmod(v0, v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(fmod(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns the floor of the argument.
 */
static ScriptObjectPointer floorf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return *argv;

        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return Integer::instance(floor(value));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns the ceiling of the argument.
 */
static ScriptObjectPointer ceiling (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return *argv;

        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return Integer::instance(ceil(value));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns the truncation of the argument.
 */
static ScriptObjectPointer truncate (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return *argv;

        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return Integer::instance(value);
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Rounds the argument to the nearest integer.
 */
static ScriptObjectPointer round (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return *argv;

        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return Integer::instance(qRound(value));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the exponential of the argument.
 */
static ScriptObjectPointer expf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(exp(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(exp(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the log of the argument.
 */
static ScriptObjectPointer logf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1 && argc != 2) {
        throw QString("Requires one or two arguments.");
    }
    switch (argv[0]->type()) {
        case ScriptObject::IntegerType: {
            int v0 = static_cast<Integer*>(argv[0].data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(log(v0)));
            }
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(log(v0) / log(v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(log(v0) / log(v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        case ScriptObject::FloatType: {
            float v0 = static_cast<Float*>(argv[0].data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(log(v0)));
            }
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(log(v0) / log(v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(log(v0) / log(v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the sine of the argument.
 */
static ScriptObjectPointer sinf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(sin(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(sin(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the cosine of the argument.
 */
static ScriptObjectPointer cosf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(cos(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(cos(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the tangent of the argument.
 */
static ScriptObjectPointer tanf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(tan(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(tan(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the asin of the argument.
 */
static ScriptObjectPointer asinf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(asin(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(asin(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the acos of the argument.
 */
static ScriptObjectPointer acosf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(acos(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(acos(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Computes the atan of the argument.
 */
static ScriptObjectPointer atanf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1 && argc != 2) {
        throw QString("Requires one or two arguments.");
    }
    switch (argv[0]->type()) {
        case ScriptObject::IntegerType: {
            int v0 = static_cast<Integer*>(argv[0].data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(atan(v0)));
            }
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(atan2(v0, v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(atan2(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        case ScriptObject::FloatType: {
            float v0 = static_cast<Float*>(argv[0].data())->value();
            if (argc == 1) {
                return ScriptObjectPointer(new Float(atan(v0)));
            }
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(atan2(v0, v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(atan2(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Calculates the square root of the argument.
 */
static ScriptObjectPointer sqrtf (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return ScriptObjectPointer(new Float(sqrt(value)));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(sqrt(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Calculates the first argument to the power of the second.
 */
static ScriptObjectPointer expt (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    switch (argv[0]->type()) {
        case ScriptObject::IntegerType: {
            int v0 = static_cast<Integer*>(argv[0].data())->value();
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(pow(v0, v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(pow(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        case ScriptObject::FloatType: {
            float v0 = static_cast<Float*>(argv[0].data())->value();
            switch (argv[1]->type()) {
                case ScriptObject::IntegerType: {
                    int v1 = static_cast<Integer*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(pow(v0, v1)));
                }
                case ScriptObject::FloatType: {
                    float v1 = static_cast<Float*>(argv[1].data())->value();
                    return ScriptObjectPointer(new Float(pow(v0, v1)));
                }
                default:
                    throw QString("Invalid argument.");
            }
        }
        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Returns whether all arguments are equal.
 */
static ScriptObjectPointer equals (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
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
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObject* number = argv->data();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if (number->greaterEquals(*arg)) {
            return Boolean::instance(false);
        }
        number = arg->data();
    }
    return Boolean::instance(true);
}

/**
 * Returns whether all arguments are monotonically decreasing.
 */
static ScriptObjectPointer greater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObject* number = argv->data();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if (number->lessEquals(*arg)) {
            return Boolean::instance(false);
        }
        number = arg->data();
    }
    return Boolean::instance(true);
}

/**
 * Returns whether all arguments are monotonically nondecreasing.
 */
static ScriptObjectPointer lessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObject* number = argv->data();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if (number->greaterThan(*arg)) {
            return Boolean::instance(false);
        }
        number = arg->data();
    }
    return Boolean::instance(true);
}

/**
 * Returns whether all arguments are monotonically nonincreasing.
 */
static ScriptObjectPointer greaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObject* number = argv->data();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if (number->lessThan(*arg)) {
            return Boolean::instance(false);
        }
        number = arg->data();
    }
    return Boolean::instance(true);
}

/**
 * Returns the maximum of the arguments.
 */
static ScriptObjectPointer max (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObjectPointer number = *argv;
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if ((*arg)->greaterThan(number)) {
            number = *arg;
        }
    }
    return number;
}

/**
 * Returns the minimum of the arguments.
 */
static ScriptObjectPointer min (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }
    if (!(*argv)->isNumber()) {
        throw QString("Invalid argument.");
    }
    ScriptObjectPointer number = *argv;
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if (!(*arg)->isNumber()) {
            throw QString("Invalid argument.");
        }
        if ((*arg)->lessThan(number)) {
            number = *arg;
        }
    }
    return number;
}

/**
 * Returns the absolute value of the argument.
 */
static ScriptObjectPointer abs (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType: {
            int value = static_cast<Integer*>(argv->data())->value();
            return Integer::instance(qAbs(value));
        }
        case ScriptObject::FloatType: {
            float value = static_cast<Float*>(argv->data())->value();
            return ScriptObjectPointer(new Float(qAbs(value)));
        }
        default:
            throw QString("Invalid argument.");
    }
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
 * Checks whether the argument is positive.
 */
static ScriptObjectPointer positive (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return Boolean::instance(static_cast<Integer*>(argv->data())->value() > 0);

        case ScriptObject::FloatType:
            return Boolean::instance(static_cast<Float*>(argv->data())->value() > 0);

        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Checks whether the argument is negative.
 */
static ScriptObjectPointer negative (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    switch ((*argv)->type()) {
        case ScriptObject::IntegerType:
            return Boolean::instance(static_cast<Integer*>(argv->data())->value() < 0);

        case ScriptObject::FloatType:
            return Boolean::instance(static_cast<Float*>(argv->data())->value() < 0);

        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Checks whether the argument is zero.
 */
static ScriptObjectPointer numberToString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }

    int radix = 10;
    int precision = 6;
    switch (argc) {
        case 3:
            if (argv[2]->type() != ScriptObject::IntegerType) {
                throw QString("Invalid argument.");
            }
            precision = static_cast<Integer*>(argv[2].data())->value();

        case 2:
            if (argv[1]->type() != ScriptObject::IntegerType) {
            }
            radix = static_cast<Integer*>(argv[1].data())->value();

        case 1:
            break;

        default:
            throw QString("Requires one, two, or three arguments.");
    }
    switch (argv[0]->type()) {
        case ScriptObject::IntegerType:
            return ScriptObjectPointer(new String(QString::number(
                static_cast<Integer*>(argv[0].data())->value(), radix)));
            break;

        case ScriptObject::FloatType:
            return ScriptObjectPointer(new String(QString::number(
                static_cast<Float*>(argv[0].data())->value(), 'g', precision)));
            break;

        default:
            throw QString("Invalid argument.");
    }
}

/**
 * Checks whether the argument is zero.
 */
static ScriptObjectPointer stringToNumber (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    int radix = 10;
    switch (argc) {
        case 2:
            if (argv[1]->type() != ScriptObject::IntegerType) {
                throw QString("Invalid argument.");
            }
            radix = static_cast<Integer*>(argv[1].data())->value();

        case 1:
            break;

        default:
            throw QString("Requires one or two arguments.");
    }
    if (argv[0]->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString& string = static_cast<String*>(argv[0].data())->contents();
    bool ok;
    if (!string.contains('.') || radix != 10) {
        int value = string.toInt(&ok, radix);
        return ok ? Integer::instance(value) : Boolean::instance(false);

    } else {
        float value = string.toDouble(&ok);
        return ok ? ScriptObjectPointer(new Float(value)) : Boolean::instance(false);
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
 * Converts a character to an integer.
 */
static ScriptObjectPointer charToInteger (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    Char* ch = static_cast<Char*>(argv->data());
    return Integer::instance(ch->value().unicode());
}

/**
 * Converts an integer to a character.
 */
static ScriptObjectPointer integerToChar (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    Integer* integer = static_cast<Integer*>(argv->data());
    return Char::instance(integer->value());
}

/**
 * Checks characters for equality.
 */
static ScriptObjectPointer charsEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv->data())->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        if (static_cast<Char*>(arg->data())->value() != ch) {
            return Boolean::instance(false);
        }
    }
    return Boolean::instance(true);
}

/**
 * Checks characters for monotonic increase.
 */
static ScriptObjectPointer charsLess (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv->data())->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        QChar nch = static_cast<Char*>(arg->data())->value();
        if (ch >= nch) {
            return Boolean::instance(false);
        }
        ch = nch;
    }
    return Boolean::instance(true);
}

/**
 * Checks characters for monotonic decrease.
 */
static ScriptObjectPointer charsGreater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv->data())->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        QChar nch = static_cast<Char*>(arg->data())->value();
        if (ch <= nch) {
            return Boolean::instance(false);
        }
        ch = nch;
    }
    return Boolean::instance(true);
}

/**
 * Checks characters for monotonic nondecrease.
 */
static ScriptObjectPointer charsLessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv->data())->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        QChar nch = static_cast<Char*>(arg->data())->value();
        if (ch > nch) {
            return Boolean::instance(false);
        }
        ch = nch;
    }
    return Boolean::instance(true);
}

/**
 * Checks characters for monotonic nonincrease.
 */
static ScriptObjectPointer charsGreaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv->data())->value();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        QChar nch = static_cast<Char*>(arg->data())->value();
        if (ch < nch) {
            return Boolean::instance(false);
        }
        ch = nch;
    }
    return Boolean::instance(true);
}

/**
 * Makes a new string.
 */
static ScriptObjectPointer makeString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    QChar fill;
    if (argc == 2) {
        if (argv[1]->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        fill = static_cast<Char*>(argv[1].data())->value();

    } else if (argc != 1) {
        throw QString("Requires one or two arguments.");
    }
    if ((*argv)->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    Integer* size = static_cast<Integer*>(argv->data());
    return ScriptObjectPointer(new String(QString(size->value(), fill)));
}

/**
 * Makes a string out of the arguments.
 */
static ScriptObjectPointer string (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    QString contents;
    for (ScriptObjectPointer* arg = argv, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::CharType) {
            throw QString("Invalid argument.");
        }
        contents.append(static_cast<Char*>(arg->data())->value());
    }
    return ScriptObjectPointer(new String(contents));
}

/**
 * Returns the length of a string.
 */
static ScriptObjectPointer stringLength (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    String* string = static_cast<String*>(argv->data());
    return Integer::instance(string->contents().size());
}

/**
 * Returns a character within a string.
 */
static ScriptObjectPointer stringRef (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[0]->type() != ScriptObject::StringType ||
            argv[1]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    const QString& contents = static_cast<String*>(argv[0].data())->contents();
    int idx = static_cast<Integer*>(argv[1].data())->value();
    if (idx < 0 || idx >= contents.size()) {
        throw QString("Invalid index.");
    }
    return ScriptObjectPointer(new Char(contents.at(idx)));
}

/**
 * Sets a character within a string.
 */
static ScriptObjectPointer stringSet (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 3) {
        throw QString("Requires exactly three arguments.");
    }
    if (argv[0]->type() != ScriptObject::StringType ||
            argv[1]->type() != ScriptObject::IntegerType ||
            argv[2]->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    String* string = static_cast<String*>(argv[0].data());
    if (!string->isMutable()) {
        throw QString("Invalid argument.");
    }
    QString& contents = string->contents();
    int idx = static_cast<Integer*>(argv[1].data())->value();
    if (idx < 0 || idx >= contents.size()) {
        throw QString("Invalid index.");
    }
    Char* ch = static_cast<Char*>(argv[2].data());
    contents[idx] = ch->value();
    return Unspecified::instance();
}

/**
 * Fills a string with some character.
 */
static ScriptObjectPointer stringFill (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[0]->type() != ScriptObject::StringType ||
            argv[1]->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    String* string = static_cast<String*>(argv[0].data());
    if (!string->isMutable()) {
        throw QString("Invalid argument.");
    }
    QChar ch = static_cast<Char*>(argv[1].data())->value();
    string->contents().fill(ch);
    return Unspecified::instance();
}

/**
 * Checks strings for equality.
 */
static ScriptObjectPointer stringsEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString& string = static_cast<String*>(argv->data())->contents();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        if (static_cast<String*>(arg->data())->contents() != string) {
            return Boolean::instance(false);
        }
    }
    return Boolean::instance(true);
}

/**
 * Checks strings for monotonic increase.
 */
static ScriptObjectPointer stringsLess (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString string = static_cast<String*>(argv->data())->contents();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        const QString& nstring = static_cast<String*>(arg->data())->contents();
        if (string >= nstring) {
            return Boolean::instance(false);
        }
        string = nstring;
    }
    return Boolean::instance(true);
}

/**
 * Checks strings for monotonic decrease.
 */
static ScriptObjectPointer stringsGreater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString string = static_cast<String*>(argv->data())->contents();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        const QString& nstring = static_cast<String*>(arg->data())->contents();
        if (string <= nstring) {
            return Boolean::instance(false);
        }
        string = nstring;
    }
    return Boolean::instance(true);
}

/**
 * Checks strings for monotonic nondecrease.
 */
static ScriptObjectPointer stringsLessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString string = static_cast<String*>(argv->data())->contents();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        const QString& nstring = static_cast<String*>(arg->data())->contents();
        if (string > nstring) {
            return Boolean::instance(false);
        }
        string = nstring;
    }
    return Boolean::instance(true);
}

/**
 * Checks strings for monotonic nonincrease.
 */
static ScriptObjectPointer stringsGreaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QString string = static_cast<String*>(argv->data())->contents();
    for (ScriptObjectPointer* arg = argv + 1, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        const QString& nstring = static_cast<String*>(arg->data())->contents();
        if (string < nstring) {
            return Boolean::instance(false);
        }
        string = nstring;
    }
    return Boolean::instance(true);
}

/**
 * Returns a substring of a string.
 */
static ScriptObjectPointer substring (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 3) {
        throw QString("Requires exactly three arguments.");
    }
    if (argv[0]->type() != ScriptObject::StringType ||
            argv[1]->type() != ScriptObject::IntegerType ||
            argv[2]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    const QString& string = static_cast<String*>(argv[0].data())->contents();
    int start = static_cast<Integer*>(argv[1].data())->value();
    int end = static_cast<Integer*>(argv[2].data())->value();
    if (start < 0 || end < start || end > string.size()) {
        throw QString("Invalid argument.");
    }
    return ScriptObjectPointer(new String(string.mid(start, end - start)));
}

/**
 * Appends strings together.
 */
static ScriptObjectPointer stringAppend (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    QString result;
    for (ScriptObjectPointer* arg = argv, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::StringType) {
            throw QString("Invalid argument.");
        }
        result.append(static_cast<String*>(arg->data())->contents());
    }
    return ScriptObjectPointer(new String(result));
}

/**
 * Converts a string to a list of characters.
 */
static ScriptObjectPointer stringToList (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    const QString& string = static_cast<String*>(argv->data())->contents();
    ScriptObjectPointer head, tail;
    for (int ii = 0, nn = string.length(); ii < nn; ii++) {
        ScriptObjectPointer ntail(new Pair(
            ScriptObjectPointer(new Char(string.at(ii)))));
        if (head.isNull()) {
            head = tail = ntail;
        } else {
            static_cast<Pair*>(tail.data())->setCdr(ntail);
            tail = ntail;
        }
    }
    if (head.isNull()) {
        return Null::instance();
    }
    static_cast<Pair*>(tail.data())->setCdr(Null::instance());
    return head;
}

/**
 * Converts a list of characters to a string.
 */
static ScriptObjectPointer listToString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    QString contents;
    for (ScriptObjectPointer rest = *argv;; ) {
        switch (rest->type()) {
            case ScriptObject::PairType: {
                Pair* pair = static_cast<Pair*>(rest.data());
                ScriptObjectPointer car = pair->car();
                if (car->type() != ScriptObject::CharType) {
                    throw QString("Invalid argument.");
                }
                contents.append(static_cast<Char*>(car.data())->value());
                rest = pair->cdr();
                break;
            }
            case ScriptObject::NullType:
                goto outerBreak;

            default:
                throw QString("Invalid argument.");
        }
    }
    outerBreak:

    return ScriptObjectPointer(new String(contents));
}

/**
 * Returns a copy of a string.
 */
static ScriptObjectPointer stringCopy (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    String* string = static_cast<String*>(argv->data());
    return ScriptObjectPointer(new String(string->contents()));
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
 * Creates a new pair from the arguments.
 */
static ScriptObjectPointer cons (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    return ScriptObjectPointer(eval->pairInstance(argv[0], argv[1]));
}

/**
 * Returns the value of the argument's car field.
 */
static ScriptObjectPointer car (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::PairType) {
        throw QString("Invalid argument.");
    }
    Pair* pair = static_cast<Pair*>(argv->data());
    return pair->car();
}

/**
 * Returns the value of the argument's cdr field.
 */
static ScriptObjectPointer cdr (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::PairType) {
        throw QString("Invalid argument.");
    }
    Pair* pair = static_cast<Pair*>(argv->data());
    return pair->cdr();
}

/**
 * Sets a pair's car field.
 */
static ScriptObjectPointer setCar (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if ((*argv)->type() != ScriptObject::PairType) {
        throw QString("Invalid argument.");
    }
    Pair* pair = static_cast<Pair*>(argv->data());
    if (!pair->isMutable()) {
        throw QString("Invalid argument.");
    }
    pair->setCar(argv[1]);
    return Unspecified::instance();
}

/**
 * Sets a pair's cdr field.
 */
static ScriptObjectPointer setCdr (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if ((*argv)->type() != ScriptObject::PairType) {
        throw QString("Invalid argument.");
    }
    Pair* pair = static_cast<Pair*>(argv->data());
    if (!pair->isMutable()) {
        throw QString("Invalid argument.");
    }
    pair->setCdr(argv[1]);
    return Unspecified::instance();
}

/**
 * Creates a new list from the arguments.
 */
static ScriptObjectPointer list (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return eval->listInstance(argv, argc);
}

/**
 * Appends the arguments together.
 */
static ScriptObjectPointer append (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc == 0) {
        throw QString("Requires at least one argument.");
    }
    ScriptObjectPointer head, tail;
    for (ScriptObjectPointer* arg = argv, *end = argv + argc; arg != end; arg++) {
        for (ScriptObjectPointer rest = *arg;; ) {
            switch (rest->type()) {
                case ScriptObject::PairType: {
                    Pair* pair = static_cast<Pair*>(rest.data());
                    if (head.isNull()) {
                        if (arg == end - 1) {
                            return *arg;
                        }
                        head = tail = eval->pairInstance(pair->car());

                    } else {
                        ScriptObjectPointer ntail = eval->pairInstance(pair->car());
                        static_cast<Pair*>(tail.data())->setCdr(ntail);
                        tail = ntail;
                    }
                    rest = pair->cdr();
                    break;
                }
                case ScriptObject::NullType:
                    goto outerBreak;

                default:
                    throw QString("Invalid argument.");
            }
        }
        outerBreak: ;
    }
    if (head.isNull()) {
        return Null::instance();
    }
    static_cast<Pair*>(tail.data())->setCdr(Null::instance());
    return head;
}

/**
 * Returns the length of a list.
 */
static ScriptObjectPointer length (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    int length = (*argv)->listLength();
    if (length == -1) {
        throw QString("Invalid argument.");
    }
    return Integer::instance(length);
}

/**
 * Reverses a list.
 */
static ScriptObjectPointer reverse (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    ScriptObjectPointer rest = *argv;
    ScriptObjectPointer head = Null::instance();
    for (ScriptObjectPointer rest = *argv;; ) {
        switch (rest->type()) {
            case ScriptObject::PairType: {
                Pair* pair = static_cast<Pair*>(rest.data());
                head = eval->pairInstance(pair->car(), head);
                rest = pair->cdr();
                break;
            }
            case ScriptObject::NullType:
                return head;

            default:
                throw QString("Invalid argument.");
        }
    }
}

/**
 * Returns the tail of the list.
 */
static ScriptObjectPointer listTail (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[1]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    int index = static_cast<Integer*>(argv[1].data())->value();
    if (index < 0) {
        throw QString("Invalid argument.");
    }
    for (ScriptObjectPointer rest = *argv;; ) {
        switch (rest->type()) {
            case ScriptObject::PairType: {
                Pair* pair = static_cast<Pair*>(rest.data());
                if (index-- == 0) {
                    return rest;
                }
                rest = pair->cdr();
                break;
            }
            case ScriptObject::NullType:
                if (index == 0) {
                    return rest;
                }
            default:
                throw QString("Invalid argument.");
        }
    }
}

/**
 * Returns an element of the list.
 */
static ScriptObjectPointer listRef (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[1]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    int index = static_cast<Integer*>(argv[1].data())->value();
    if (index < 0) {
        throw QString("Invalid argument.");
    }
    for (ScriptObjectPointer rest = *argv;; ) {
        switch (rest->type()) {
            case ScriptObject::PairType: {
                Pair* pair = static_cast<Pair*>(rest.data());
                if (index-- == 0) {
                    return pair->car();
                }
                rest = pair->cdr();
                break;
            }
            default:
                throw QString("Invalid argument.");
        }
    }
}

/**
 * Creates a new vector.
 */
static ScriptObjectPointer makeVector (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    ScriptObjectPointer fill = Unspecified::instance();
    if (argc == 2) {
        fill = argv[1];

    } else if (argc != 1) {
        throw QString("Requires one or two arguments.");
    }
    if ((*argv)->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    Integer* size = static_cast<Integer*>(argv->data());
    ScriptObjectPointerVector contents;
    for (int ii = 0, nn = size->value(); ii < nn; ii++) {
        contents.append(fill);
    }
    return eval->vectorInstance(contents);
}

/**
 * Creates a new vector from the arguments.
 */
static ScriptObjectPointer vector (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    ScriptObjectPointerVector contents;
    for (int ii = 0; ii < argc; ii++) {
        contents.append(argv[ii]);
    }
    return eval->vectorInstance(contents);
}

/**
 * Returns the length of a vector.
 */
static ScriptObjectPointer vectorLength (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::VectorType) {
        throw QString("Invalid argument.");
    }
    Vector* vector = static_cast<Vector*>(argv->data());
    return Integer::instance(vector->contents().size());
}

/**
 * Returns an element of the vector.
 */
static ScriptObjectPointer vectorRef (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[0]->type() != ScriptObject::VectorType ||
            argv[1]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    ScriptObjectPointerVector& contents = static_cast<Vector*>(argv[0].data())->contents();
    int idx = static_cast<Integer*>(argv[1].data())->value();
    if (idx < 0 || idx >= contents.size()) {
        throw QString("Invalid index.");
    }
    return contents.at(idx);
}

/**
 * Sets an element of the vector.
 */
static ScriptObjectPointer vectorSet (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 3) {
        throw QString("Requires exactly three arguments.");
    }
    if (argv[0]->type() != ScriptObject::VectorType ||
            argv[1]->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    Vector* vector = static_cast<Vector*>(argv[0].data());
    if (!vector->isMutable()) {
        throw QString("Invalid argument.");
    }
    ScriptObjectPointerVector& contents = vector->contents();
    int idx = static_cast<Integer*>(argv[1].data())->value();
    if (idx < 0 || idx >= contents.size()) {
        throw QString("Invalid index.");
    }
    contents.replace(idx, argv[2]);
    return Unspecified::instance();
}

/**
 * Converts a vector to a list.
 */
static ScriptObjectPointer vectorToList (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::VectorType) {
        throw QString("Invalid argument.");
    }
    const ScriptObjectPointerVector& contents = static_cast<Vector*>(argv->data())->contents();
    return eval->listInstance(contents.constData(), contents.size());
}

/**
 * Converts a list to a vector.
 */
static ScriptObjectPointer listToVector (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    bool ok;
    ScriptObjectPointerVector contents = (*argv)->listContents(&ok);
    if (!ok) {
        throw QString("Invalid argument.");
    }
    return eval->vectorInstance(contents);
}

/**
 * Fills the vector.
 */
static ScriptObjectPointer vectorFill (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if ((*argv)->type() != ScriptObject::VectorType) {
        throw QString("Invalid argument.");
    }
    Vector* vector = static_cast<Vector*>(argv->data());
    if (!vector->isMutable()) {
        throw QString("Invalid argument.");
    }
    qFill(vector->contents(), argv[1]);
    return Unspecified::instance();
}

/**
 * Appends the argument vectors together.
 */
static ScriptObjectPointer vectorAppend (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    ScriptObjectPointerVector contents;
    for (ScriptObjectPointer* arg = argv, *end = argv + argc; arg != end; arg++) {
        if ((*arg)->type() != ScriptObject::VectorType) {
            throw QString("Invalid argument.");
        }
        Vector* vector = static_cast<Vector*>(arg->data());
        contents += vector->contents();
    }
    return eval->vectorInstance(contents);
}

/**
 * Determines whether the argument is a pair.
 */
static ScriptObjectPointer pair (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::PairType);
}

/**
 * Determines whether the argument is the empty list.
 */
static ScriptObjectPointer null (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::NullType);
}

/**
 * Determines whether the argument is a list.
 */
static ScriptObjectPointer listp (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->list());
}

/**
 * Determines whether the argument is a vector.
 */
static ScriptObjectPointer vectorp (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::VectorType);
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
    return Boolean::instance((*argv)->isNumber());
}

/**
 * Determines whether the argument is an integer.
 */
static ScriptObjectPointer integer (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::IntegerType);
}

/**
 * Determines whether the argument is a character.
 */
static ScriptObjectPointer charp (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    return Boolean::instance((*argv)->type() == ScriptObject::CharType);
}

/**
 * Determines whether the argument is a string.
 */
static ScriptObjectPointer stringp (Evaluator* eval, int argc, ScriptObjectPointer* argv)
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
 * Prints the arguments as debug output.
 */
static ScriptObjectPointer debug (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    {
        QDebug debug(QtDebugMsg);
        for (ScriptObjectPointer* arg = argv, *end = argv + argc; arg != end; arg++) {
            debug << (*arg)->toString();
        }
    }
    return Unspecified::instance();
}

/**
 * Continues execution after some number of milliseconds have elapsed.
 */
static ScriptObjectPointer sleep (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::IntegerType) {
        throw QString("Invalid argument.");
    }
    int value = static_cast<Integer*>(argv->data())->value();
    if (value < 0) {
        throw QString("Invalid argument.");
    }
    if (eval->maxCyclesPerSlice() == -1) {
        QMutex mutex;
        mutex.lock();
        QWaitCondition().wait(&mutex, value);
        return Unspecified::instance();

    } else {
        QTimer* timer = new QTimer(eval);
        eval->connect(timer, SIGNAL(timeout()), SLOT(wakeUp()));
        timer->connect(timer, SIGNAL(timeout()), SLOT(deleteLater()));
        timer->connect(eval, SIGNAL(canceled()), SLOT(deleteLater()));
        timer->setSingleShot(true);
        timer->start(value);

        eval->setWaker(timer);
        return ScriptObjectPointer();
    }
}

/**
 * Exits the application.
 */
static ScriptObjectPointer exit (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    int code = 0;
    if (argc == 1) {
        ScriptObject::Type type = (*argv)->type();
        if (type == ScriptObject::BooleanType) {
            code = static_cast<Boolean*>(argv->data())->value() ? 0 : 1;

        } else if (type == ScriptObject::IntegerType) {
            code = static_cast<Integer*>(argv->data())->value();

        } else {
            throw QString("Expected boolean or integer return value.");
        }
    } else if (argc != 0) {
        throw QString("Requires zero or one arguments.");
    }
    QCoreApplication::exit(code);
    return Unspecified::instance();
}

/**
 * Returns a wrapped pointer to the evaluator.
 */
static ScriptObjectPointer evaluator (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 0) {
        throw QString("Takes no arguments.");
    }
    return ScriptObjectPointer(new WrappedObject(eval));
}

/**
 * Returns a wrapped pointer to the parent of the argument, or null.
 */
static ScriptObjectPointer parent (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::WrappedObjectType) {
        throw QString("Invalid argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv->data())->object()->parent();
    return object == 0 ? Null::instance() : ScriptObjectPointer(new WrappedObject(object));
}

/**
 * Returns a list of the children of the argument.
 */
static ScriptObjectPointer children (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    if ((*argv)->type() != ScriptObject::WrappedObjectType) {
        throw QString("Invalid argument.");
    }
    const QObjectList& children = static_cast<WrappedObject*>(argv->data())->object()->children();
    ScriptObjectPointer head, tail;
    foreach (QObject* child, children) {
        ScriptObjectPointer ntail = eval->pairInstance(ScriptObjectPointer(new WrappedObject(child)));
        if (head.isNull()) {
            head = tail = ntail;

        } else {
            static_cast<Pair*>(tail.data())->setCdr(ntail);
            tail = ntail;
        }
    }
    if (head.isNull()) {
        return Null::instance();
    }
    static_cast<Pair*>(tail.data())->setCdr(Null::instance());
    return head;
}

/**
 * Converts a variant value to a script object.
 */
static ScriptObjectPointer variantToScriptObject (Evaluator* eval, const QVariant& variant)
{
    switch (variant.userType()) {
        case QVariant::Bool:
            return Boolean::instance(variant.toBool());

        case QVariant::ByteArray:
            return ScriptObjectPointer(new ByteVector(variant.toByteArray()));

        case QVariant::Char:
            return Char::instance(variant.toChar());

        case QVariant::Double:
        case QMetaType::Float:
            return ScriptObjectPointer(new Float(variant.toDouble()));

        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            return Integer::instance(variant.toInt());

        case QVariant::List:
        case QVariant::StringList: {
            ScriptObjectPointerVector contents;
            foreach (const QVariant& variant, variant.toList()) {
                contents.append(variantToScriptObject(eval, variant));
            }
            return eval->vectorInstance(contents);
        }
        case QVariant::String:
            return ScriptObjectPointer(new String(variant.toString()));

        case QMetaType::QObjectStar:
            return ScriptObjectPointer(new WrappedObject(variant.value<QObject*>()));

        default:
            return Null::instance();
    }
}

/**
 * Retrieves a property of a wrapped object.
 */
static ScriptObjectPointer property (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[0]->type() != ScriptObject::WrappedObjectType ||
            argv[1]->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv[0].data())->object();
    const QString& name = static_cast<String*>(argv[1].data())->contents();
    return variantToScriptObject(eval, object->property(name.toAscii()));
}

/**
 * Converts a script object to a variant.
 */
static QVariant scriptObjectToVariant (const ScriptObjectPointer& object)
{
    switch (object->type()) {
        case ScriptObject::BooleanType:
            return QVariant(static_cast<Boolean*>(object.data())->value());

        case ScriptObject::ByteVectorType:
            return QVariant(static_cast<ByteVector*>(object.data())->contents());

        case ScriptObject::CharType:
            return QVariant(static_cast<Char*>(object.data())->value());

        case ScriptObject::FloatType:
            return QVariant(static_cast<Float*>(object.data())->value());

        case ScriptObject::IntegerType:
            return QVariant(static_cast<Integer*>(object.data())->value());

        case ScriptObject::VectorType: {
            QVariantList list;
            const ScriptObjectPointerVector& contents =
                static_cast<Vector*>(object.data())->contents();
            for (int ii = 0, nn = contents.size(); ii < nn; ii++) {
                list.append(scriptObjectToVariant(contents.at(ii)));
            }
            return QVariant(list);
        }
        case ScriptObject::StringType:
            return QVariant(static_cast<String*>(object.data())->contents());

        case ScriptObject::WrappedObjectType:
            return QVariant::fromValue(static_cast<WrappedObject*>(object.data())->object());

        default:
            return QVariant();
    }
}

/**
 * Sets a property of a wrapped object.
 */
static ScriptObjectPointer setProperty (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 3) {
        throw QString("Requires exactly three arguments.");
    }
    if (argv[0]->type() != ScriptObject::WrappedObjectType ||
            argv[1]->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv[0].data())->object();
    const QString& name = static_cast<String*>(argv[1].data())->contents();
    return Boolean::instance(object->setProperty(name.toAscii(), scriptObjectToVariant(argv[2])));
}

/**
 * Invokes a method of a wrapped object.
 */
static ScriptObjectPointer invokeMethod (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc < 2) {
        throw QString("Requires at least two arguments.");
    }
    if (argv[0]->type() != ScriptObject::WrappedObjectType ||
            argv[1]->type() != ScriptObject::StringType) {
        throw QString("Invalid argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv[0].data())->object();
    const QString& name = static_cast<String*>(argv[1].data())->contents();
    QVariantList variants;
    QGenericArgument args[10];
    int idx = 0;
    for (ScriptObjectPointer* arg = argv + 2, *end = argv + argc; arg != end; arg++) {
        variants.append(scriptObjectToVariant(*arg));
        const QVariant& variant = variants.at(idx);
        args[idx++] = QGenericArgument(variant.typeName(), variant.constData());
    }
    return Boolean::instance(QMetaObject::invokeMethod(object, name.toAscii(),
        args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9]));
}

/**
 * Returns the evaluator's standard input port.
 */
static ScriptObjectPointer standardInputPort (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 0) {
        throw QString("Takes no arguments.");
    }
    return eval->standardInputPort();
}

/**
 * Returns the evaluator's standard output port.
 */
static ScriptObjectPointer standardOutputPort (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 0) {
        throw QString("Takes no arguments.");
    }
    return eval->standardOutputPort();
}

/**
 * Returns the evaluator's standard error port.
 */
static ScriptObjectPointer standardErrorPort (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 0) {
        throw QString("Takes no arguments.");
    }
    return eval->standardErrorPort();
}

/**
 * Writes a character to a port.
 */
static ScriptObjectPointer putChar (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 2) {
        throw QString("Requires exactly two arguments.");
    }
    if (argv[0]->type() != ScriptObject::WrappedObjectType ||
            argv[1]->type() != ScriptObject::CharType) {
        throw QString("Invalid argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv[0].data())->object();
    QIODevice* device = qobject_cast<QIODevice*>(object);
    if (device == 0 || !device->isWritable()) {
        throw QString("Invalid argument.");
    }
    QTextStream(device) << static_cast<Char*>(argv[1].data())->value();
    return Unspecified::instance();
}

/**
 * Reads a line of input from a port.
 */
static ScriptObjectPointer getLine (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    if (argc != 1) {
        throw QString("Requires exactly one argument.");
    }
    QObject* object = static_cast<WrappedObject*>(argv[0].data())->object();
    QIODevice* device = qobject_cast<QIODevice*>(object);
    if (device == 0 || !device->isReadable()) {
        throw QString("Invalid argument.");
    }
    if (device->canReadLine()) {
        return ScriptObjectPointer(new String(device->readLine()));
    }
    eval->setWaker(device);
    eval->connect(device, SIGNAL(readyRead()), SLOT(maybeReadLine()));
    return ScriptObjectPointer();
}

/**
 * Creates the global scope object.
 */
static Scope createGlobalScope ()
{
    Scope scope(0, true);

    scope.addVariable("eqv?", eqv);
    scope.addVariable("eq?", eqv);
    scope.addVariable("equal?", equal);

    scope.addVariable("+", add);
    scope.addVariable("-", subtract);
    scope.addVariable("*", multiply);
    scope.addVariable("/", divide);
    scope.addVariable("mod", mod);
    scope.addVariable("floor", floorf);
    scope.addVariable("ceiling", ceiling);
    scope.addVariable("truncate", truncate);
    scope.addVariable("round", round);

    scope.addVariable("exp", expf);
    scope.addVariable("log", logf);
    scope.addVariable("sin", sinf);
    scope.addVariable("cos", cosf);
    scope.addVariable("tan", tanf);
    scope.addVariable("asin", asinf);
    scope.addVariable("acos", acosf);
    scope.addVariable("atan", atanf);

    scope.addVariable("sqrt", sqrtf);
    scope.addVariable("expt", expt);

    scope.addVariable("=", equals);
    scope.addVariable("<", less);
    scope.addVariable(">", greater);
    scope.addVariable("<=", lessEqual);
    scope.addVariable(">=", greaterEqual);

    scope.addVariable("max", max);
    scope.addVariable("min", min);
    scope.addVariable("abs", abs);

    scope.addVariable("zero?", zero);
    scope.addVariable("positive?", positive);
    scope.addVariable("negative?", negative);

    scope.addVariable("number->string", numberToString);
    scope.addVariable("string->number", stringToNumber);

    scope.addVariable("not", notfunc);
    scope.addVariable("boolean=?", booleansEqual);

    scope.addVariable("char->integer", charToInteger);
    scope.addVariable("integer->char", integerToChar);
    scope.addVariable("char=?", charsEqual);
    scope.addVariable("char<?", charsLess);
    scope.addVariable("char>?", charsGreater);
    scope.addVariable("char<=?", charsLessEqual);
    scope.addVariable("char>=?", charsGreaterEqual);

    scope.addVariable("make-string", makeString);
    scope.addVariable("string", string);
    scope.addVariable("string-length", stringLength);
    scope.addVariable("string-ref", stringRef);
    scope.addVariable("string-set!", stringSet);
    scope.addVariable("string-fill!", stringFill);
    scope.addVariable("string=?", stringsEqual);
    scope.addVariable("string<?", stringsLess);
    scope.addVariable("string>?", stringsGreater);
    scope.addVariable("string<=?", stringsLessEqual);
    scope.addVariable("string>=?", stringsGreaterEqual);
    scope.addVariable("substring", substring);
    scope.addVariable("string-append", stringAppend);
    scope.addVariable("string->list", stringToList);
    scope.addVariable("list->string", listToString);
    scope.addVariable("string-copy", stringCopy);

    scope.addVariable("symbol->string", symbolToString);
    scope.addVariable("string->symbol", stringToSymbol);
    scope.addVariable("symbol=?", symbolsEqual);

    scope.addVariable("cons", cons);
    scope.addVariable("car", car);
    scope.addVariable("cdr", cdr);
    scope.addVariable("set-car!", setCar);
    scope.addVariable("set-cdr!", setCdr);

    scope.addVariable("list", ScriptObjectPointer(), listProcedure());
    scope.addVariable("append", ScriptObjectPointer(), appendProcedure());
    scope.addVariable("length", length);
    scope.addVariable("reverse", reverse);
    scope.addVariable("list-tail", listTail);
    scope.addVariable("list-ref", listRef);

    scope.addVariable("make-vector", makeVector);
    scope.addVariable("vector", ScriptObjectPointer(), vectorProcedure());
    scope.addVariable("vector-length", vectorLength);
    scope.addVariable("vector-ref", vectorRef);
    scope.addVariable("vector-set!", vectorSet);
    scope.addVariable("vector->list", vectorToList);
    scope.addVariable("list->vector", listToVector);
    scope.addVariable("vector-fill!", vectorFill);
    scope.addVariable("vector-append", ScriptObjectPointer(), vectorAppendProcedure());

    scope.addVariable("pair?", pair);
    scope.addVariable("null?", null);
    scope.addVariable("list?", listp);
    scope.addVariable("vector?", vectorp);
    scope.addVariable("boolean?", boolean);
    scope.addVariable("symbol?", symbol);
    scope.addVariable("number?", number);
    scope.addVariable("integer?", integer);
    scope.addVariable("char?", charp);
    scope.addVariable("string?", stringp);
    scope.addVariable("procedure?", procedure);

    ScriptObjectPointer apply(new ApplyProcedure());
    scope.addVariable("apply", ScriptObjectPointer(), apply);

    ScriptObjectPointer callcc(new CaptureProcedure());
    scope.addVariable("call-with-current-continuation", ScriptObjectPointer(), callcc);
    scope.addVariable("call/cc", ScriptObjectPointer(), callcc);

    scope.addVariable("debug", debug);
    scope.addVariable("sleep", sleep);
    scope.addVariable("exit", exit);

    scope.addVariable("evaluator", evaluator);
    scope.addVariable("parent", parent);
    scope.addVariable("children", children);
    scope.addVariable("property", property);
    scope.addVariable("set-property!", setProperty);
    scope.addVariable("invoke-method", invokeMethod);

    scope.addVariable("standard-input-port", standardInputPort);
    scope.addVariable("standard-output-port", standardOutputPort);
    scope.addVariable("standard-error-port", standardErrorPort);
    scope.addVariable("put-char", putChar);
    scope.addVariable("get-line", getLine);

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

const ScriptObjectPointer& vectorProcedure ()
{
    static ScriptObjectPointer proc(new NativeProcedure(vector));
    return proc;
}

const ScriptObjectPointer& vectorAppendProcedure ()
{
    static ScriptObjectPointer proc(new NativeProcedure(vectorAppend));
    return proc;
}
