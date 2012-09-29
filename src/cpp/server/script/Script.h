//
// $Id$

#ifndef SCRIPT
#define SCRIPT

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QVector>

/**
 * Contains a position within a script.
 */
class ScriptPosition
{
public:

    /**
     * Creates a new position.
     */
    ScriptPosition (const QString& expr = QString(), const QString& source = QString(),
        int idx = 0, int line = 0, int lineIdx = 0);

    /**
     * Returns a string representation of the position.
     */
    QString toString (bool compact = false) const;

protected:

    /** The expression in string form. */
    QString _expr;

    /** The source of the expression. */
    QString _source;

    /** The index within the string. */
    int _idx;

    /** The line number. */
    int _line;

    /** The index at which the line starts. */
    int _lineIdx;
};

/**
 * Contains information on an error encountered in script evaluation.
 */
class ScriptError
{
public:

    /**
     * Creates a new script error with a single position.
     */
    ScriptError (const QString& message, const ScriptPosition& position);

    /**
     * Creates a new script error with a list of positions.
     */
    ScriptError (const QString& message, const QVector<ScriptPosition>& positions);

    /**
     * Returns a string representation of the error.
     */
    QString toString (bool compact = false) const;

protected:

    /** A message explaining the error. */
    QString _message;

    /** The location(s) of the error. */
    QVector<ScriptPosition> _positions;
};

/** The bytecode instructions. */
enum BytecodeOp {

    /** Pushes the constant at the specified index onto the stack. */
    ConstantOp,

    /** Pushes the variable at the specified scope/index onto the stack. */
    VariableOp,

    /** Pops a value off the stack and assigns it to the variable at the specified scope/index. */
    SetVariableOp,

    /** Pushes the current operand count and sets the count to zero. */
    ResetOperandCountOp,

    /** Performs a function call. */
    CallOp,

    /** Performs a tail function call. */
    TailCallOp,

    /** Returns from a function call. */
    ReturnOp,

    /** Creates a function instance. */
    LambdaOp,

    /** Pops a value off the stack and discards it. */
    PopOp,
    
    /** Returns from the current execution cycle with the value on the stack as a result. */
    ExitOp,
    
    /** Returns from the current execution cycle with Unspecified as a result. */
    LambdaExitOp,
    
    /** Jumps to the specified offset. */
    JumpOp,
    
    /** Pops a value off the stack; if it's false, jumps to the specified offset. */
    ConditionalJumpOp
};

/**
 * Contains bytecode data and position mappings.
 */
class Bytecode
{
public:
    
    /**
     * Returns the length of the bytecode.
     */
    int length () const { return _data.length(); }

    /**
     * Checks whether the bytecode data is empty.
     */
    bool isEmpty () const { return _data.isEmpty(); }

    /**
     * Clears the bytecode data.
     */
    void clear () { _data.clear(); _positions.clear(); }
    
    /**
     * Returns a pointer to the bytecode data.
     */
    const uchar* data () const { return (uchar*)_data.constData(); }

    /**
     * Returns the position corresponding to the specified location (or an empty position if
     * none).
     */
    ScriptPosition position (const uchar* ptr) const { return _positions.value(ptr - data()); }
    
    /**
     * Appends a simple instruction to the data.
     */
    void append (BytecodeOp op) { _data.append(op); }

    /**
     * Appends an instruction with a single integer parameter.
     */
    void append (BytecodeOp op, int p1);

    /**
     * Appends an instruction with two integer parameters.
     */
    void append (BytecodeOp op, int p1, int p2);

    /**
     * Appends bytecode data to this.
     */
    void append (const Bytecode& bytecode);

    /**
     * Associates the last instruction entered with the provided position.
     */
    void associate (const ScriptPosition& pos) { _positions.insert(_data.size(), pos); }

protected:
    
    /** The bytecode data. */
    QByteArray _data;
    
    /** Maps bytecode indices to positions. */
    QHash<int, ScriptPosition> _positions;
};

/**
 * Contains the "registers" that, in addition to the stack, comprise the evaluator state.
 */
class Registers
{
public:

    /** The index of the current invocation on the stack. */
    int invocationIdx;

    /** The pointer to the next instruction in the procedure definition. */
    const uchar* instruction;

    /** The current operand count. */
    int operandCount;
};

#endif // SCRIPT
