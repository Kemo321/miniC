#ifndef MINIC_CODEGENERATOR_HPP
#define MINIC_CODEGENERATOR_HPP

#include "minic/IRGenerator.hpp"
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <iostream>

namespace minic
{

/**
 * @class CodeGenerator
 * @brief Generate target code (textual assembly) from IRProgram.
 *
 * CodeGenerator takes an IRProgram produced by the IRGenerator and emits a
 * textual representation (or writes to a file). It maintains per-function
 * state such as stack allocation offsets, label mapping and the current
 * output stream.
 */
class CodeGenerator
{
public:
    /**
     * @brief Construct a CodeGenerator with the given output stream.
     *
     * The provided stream is used as the default place to emit generated
     * code. The constructor initializes internal mappings and state.
     *
     * @param out Output stream to write generated code to (defaults to std::cout).
     */
    explicit CodeGenerator(std::ostream& out = std::cout);

    /**
     * @brief Generate code for the given IR program.
     *
     * Traverses the IRProgram and emits code for each function and block.
     * Optionally writes the result to an output file if output_file is not empty.
     *
     * @param ir_program IR representation to generate code from.
     * @param output_file Optional path to write the emitted code into.
     */
    void generate(const IRProgram& ir_program, const std::string& output_file = "");

private:
    /**
     * @brief Emit the entire program (all functions and global data).
     *
     * Called by generate() to handle program-level emission.
     *
     * @param program IRProgram to emit.
     */
    void emit_program(const IRProgram& program);

    /**
     * @brief Emit a single IRFunction.
     *
     * Outputs function prologue/epilogue and emits its basic blocks.
     *
     * @param func Function IR to emit.
     */
    void emit_function(const IRFunction& func);

    /**
     * @brief Emit a basic block.
     *
     * Writes the block label (if needed) and emits contained instructions in order.
     *
     * @param block BasicBlock to emit.
     */
    void emit_block(const BasicBlock& block);

    /**
     * @brief Emit a single IR instruction.
     *
     * Translates an IRInstruction into one or more target assembly/text lines
     * and writes them to the output stream.
     *
     * @param instr Instruction to emit.
     */
    void emit_instruction(const IRInstruction& instr);

    /**
     * @brief Allocate stack space for function-local variables.
     *
     * Computes offsets for locals, updates stack_offset_ and var_offsets_
     * so subsequent instructions refer to correct stack locations.
     *
     * @param func Function whose stack frame to allocate.
     */
    void allocate_stack(const IRFunction& func);

    /**
     * @brief Get the textual location for a variable name.
     *
     * Returns a string describing where the named variable is stored
     * (e.g., a stack reference or register name) based on current offsets.
     *
     * @param name Variable name or temporary.
     * @return Textual location used in emitted code.
     */
    std::string get_loc(const std::string& name);

    /**
     * @brief Find a label that contains the provided substring.
     *
     * Useful for heuristics when mapping control-flow targets back to labels.
     *
     * @param substr Substring to search for inside known labels.
     * @return Matching label name, or empty string if none found.
     */
    std::string find_label_with_substr(const std::string& substr) const;

    /**
     * @brief Infer the branch target label for the current block.
     *
     * Uses block ordering and label maps to determine the most reasonable
     * fall-through or explicit target for branches emitted from the current block.
     *
     * @return Inferred label name for the current block's primary target.
     */
    std::string infer_target_label_for_current_block() const;

    std::ostream* out_; ///< Output stream used for emitted code.
    std::unordered_map<TokenType, std::string> type_map_; ///< Mapping IR types to textual types.
    std::string current_function_; ///< Name of the function currently being emitted.
    std::string current_block_label_; ///< Label of the current basic block.
    int stack_offset_; ///< Current stack offset for locals within the active function.
    std::unordered_map<std::string, int> var_offsets_; ///< Map from variable name to stack offset.
    std::vector<std::string> block_labels_; ///< Ordered list of block labels for the current function.
    std::unordered_map<std::string, size_t> block_index_; ///< Mapping block label -> index in block_labels_.
    std::unordered_set<std::string> labels_; ///< Set of labels already emitted/known.
    std::string last_written_loc_; ///< Last emitted location string (to avoid redundant moves).

    friend class PublicCodeGenerator;
};

} // namespace minic

#endif // MINIC_CODEGENERATOR_HPP