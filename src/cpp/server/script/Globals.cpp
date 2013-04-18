//
// $Id$

#include <QCoreApplication>
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
    return Unspecified::instance();
}

/**
 * Checks characters for monotonic increase.
 */
static ScriptObjectPointer charsLess (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks characters for monotonic decrease.
 */
static ScriptObjectPointer charsGreater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks characters for monotonic nondecrease.
 */
static ScriptObjectPointer charsLessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks characters for monotonic nonincrease.
 */
static ScriptObjectPointer charsGreaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Makes a new string.
 */
static ScriptObjectPointer makeString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Makes a string out of the arguments.
 */
static ScriptObjectPointer string (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Returns the length of a string.
 */
static ScriptObjectPointer stringLength (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Returns a character within a string.
 */
static ScriptObjectPointer stringRef (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks strings for equality.
 */
static ScriptObjectPointer stringsEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks strings for monotonic increase.
 */
static ScriptObjectPointer stringsLess (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks strings for monotonic decrease.
 */
static ScriptObjectPointer stringsGreater (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks strings for monotonic nondecrease.
 */
static ScriptObjectPointer stringsLessEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Checks strings for monotonic nonincrease.
 */
static ScriptObjectPointer stringsGreaterEqual (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Returns a substring of a string.
 */
static ScriptObjectPointer substring (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Appends strings together.
 */
static ScriptObjectPointer stringAppend (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Converts a string to a list of characters.
 */
static ScriptObjectPointer stringToList (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
}

/**
 * Converts a list of characters to a string.
 */
static ScriptObjectPointer listToString (Evaluator* eval, int argc, ScriptObjectPointer* argv)
{
    return Unspecified::instance();
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
    ScriptObjectPointerVector& contents = static_cast<Vector*>(argv[0].data())->contents();
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
    ScriptObject::Type type = (*argv)->type();
    return Boolean::instance(type == ScriptObject::IntegerType || type == ScriptObject::FloatType);
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
    scope.addVariable("char?", charp);
    scope.addVariable("string?", stringp);
    scope.addVariable("procedure?", procedure);

    ScriptObjectPointer apply(new ApplyProcedure());
    scope.addVariable("apply", ScriptObjectPointer(), apply);

    ScriptObjectPointer callcc(new CaptureProcedure());
    scope.addVariable("call-with-current-continuation", ScriptObjectPointer(), callcc);
    scope.addVariable("call/cc", ScriptObjectPointer(), callcc);

    scope.addVariable("debug", debug);
    scope.addVariable("exit", exit);

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
