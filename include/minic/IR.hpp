#ifndef MINIC_IR_HPP
#define MINIC_IR_HPP

#include "AST.hpp"
#include <memory>
#include <string>
#include <vector>

namespace minic
{

/**
 * @brief Intermediate Representation (IR) opcodes for miniC.
 *
 * Represents a compact set of operations used by the IR layer:
 * arithmetic, comparisons, assignments, memory access, control flow, and labels.
 */
enum class IROpcode
{
    ADD,
    SUB,
    MUL,
    DIV, // Arithmetic
    EQ,
    NEQ,
    LT,
    GT,
    LE,
    GE, // Comparisons
    ASSIGN, // Assignment
    LOAD,
    STORE, // Variable access
    JUMP,
    JUMPIF, // Control flow
    RETURN, // Return
    LABEL // Block label
};

/**
 * @class IRInstruction
 * @brief A single IR instruction.
 *
 * An IR instruction has an opcode and up to two operands plus an optional
 * result (used for temporary variables or label names).
 */
class IRInstruction
{
public:
    IROpcode opcode; ///< The opcode for this instruction
    std::string result; ///< Destination (temp var or label)
    std::string operand1; ///< First operand (or sole operand)
    std::string operand2; ///< Second operand (for binary ops)

    /**
     * @brief Construct an IRInstruction.
     * @param op The opcode.
     * @param res Optional result name.
     * @param op1 Optional first operand.
     * @param op2 Optional second operand.
     */
    IRInstruction(IROpcode op,
        const std::string& res = {},
        const std::string& op1 = {},
        const std::string& op2 = {})
        : opcode(op)
        , result(res)
        , operand1(op1)
        , operand2(op2)
    {
    }
};

/**
 * @class BasicBlock
 * @brief A sequence of IR instructions with a label.
 *
 * Basic blocks group instructions and are the unit of control-flow in the IR.
 */
class BasicBlock
{
public:
    std::string label; ///< Unique label for the block
    std::vector<IRInstruction> instructions; ///< Instructions contained in the block

    /**
     * @brief Construct a BasicBlock with the given label.
     * @param lbl The block label.
     */
    explicit BasicBlock(const std::string& lbl)
        : label(lbl)
    {
    }
};

/**
 * @class IRFunction
 * @brief Represents a function in the IR.
 *
 * Stores function name, return type, parameter list, and a sequence of basic blocks.
 */
class IRFunction
{
public:
    std::string name; ///< Function name
    TokenType return_type; ///< Function return type (from AST/Token)
    std::vector<Parameter> parameters; ///< Function parameters
    std::vector<std::unique_ptr<BasicBlock>> blocks; ///< Owned basic blocks

    /**
     * @brief Construct an IRFunction.
     * @param n Function name.
     * @param rt Return type.
     * @param params Parameter list (moved).
     */
    IRFunction(const std::string& n, TokenType rt, std::vector<Parameter> params)
        : name(n)
        , return_type(rt)
        , parameters(std::move(params))
    {
    }
};

/**
 * @class IRProgram
 * @brief A container for all IR functions comprising the program.
 */
class IRProgram
{
public:
    std::vector<std::unique_ptr<IRFunction>> functions; ///< Owned functions
};

} // namespace minic

#endif // MINIC_IR_HPP