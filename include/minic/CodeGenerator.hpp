#ifndef MINIC_CODE_GENERATOR_HPP
#define MINIC_CODE_GENERATOR_HPP

#include "minic/IR.hpp"
#include <fstream>
#include <map>
#include <string>

namespace minic {

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including the CodeGenerator that
 *        emits assembly/text output from the intermediate representation (IR).
 */

/**
 * @class CodeGenerator
 * @brief Generates target assembly (or textual) output from a miniC IRProgram.
 *
 * The CodeGenerator traverses an IRProgram and emits textual output to a file stream.
 * It handles function and block emission, instruction lowering, simple temporary-to-register
 * mapping, and local stack allocation bookkeeping.
 */
class CodeGenerator {
public:
    /**
     * @brief Generate output for the given IR program into the specified file.
     * @param ir_program The IR program to generate code for.
     * @param output_file Path to the output file to write the generated code.
     */
    void generate(const IRProgram& ir_program, const std::string& output_file);

private:
    /**
     * @brief Output stream used to write generated code.
     */
    std::ofstream out_;

    /**
     * @brief Mapping from miniC token types (used for declarations) to target directives.
     *
     * For example, integer types may map to a data allocation directive ("dq" for 64-bit).
     * TokenType values come from the IR/Token definitions.
     */
    std::map<TokenType, std::string> type_map_ = {
        {TokenType::KEYWORD_INT, "dq"},  // 64-bit int
        {TokenType::KEYWORD_VOID, ""}    // No allocation
    };

    /**
     * @brief Current stack offset (in bytes) used when allocating locals.
     *
     * This is a running total used to compute per-variable offsets on the stack.
     */
    int stack_offset_ = 0;  // Track stack for locals

    /**
     * @brief Maps variable names to their stack offsets (negative offsets relative to base/frame).
     */
    std::map<std::string, int> var_offsets_;  // Var to stack offset

    /**
     * @brief Emit an entire IR program (top-level traversal).
     * @param program The IRProgram to emit.
     */
    void emit_program(const IRProgram& program);

    /**
     * @brief Emit code for a single function IR node.
     * @param func The IRFunction to emit.
     *
     * Responsible for function prologue/epilogue, stack allocation, and emitting blocks.
     */
    void emit_function(const IRFunction& func);

    /**
     * @brief Emit code for a basic block.
     * @param block The BasicBlock to emit.
     *
     * Handles block labels and emits contained instructions in order.
     */
    void emit_block(const BasicBlock& block);

    /**
     * @brief Lower a single IR instruction to target output.
     * @param instr The IRInstruction to lower/emit.
     */
    void emit_instruction(const IRInstruction& instr);

    /**
     * @brief Map a temporary name (SSA/temp) to a target register or spill location string.
     * @param temp The temporary identifier from IR.
     * @return A string representing the register or memory operand to use in emitted code.
     *
     * This provides a simple mapping for temps to textual registers or stack locations.
     */
    std::string reg_for_temp(const std::string& temp);

    /**
     * @brief Allocate stack space for locals used by a function and populate var_offsets_.
     * @param func The function whose locals are being allocated.
     *
     * Computes stack_offset_ and var_offsets_ to be used when emitting instructions that
     * reference local variables.
     */
    void allocate_stack(const IRFunction& func);
};

} // namespace minic

#endif // MINIC_CODE_GENERATOR_HPP