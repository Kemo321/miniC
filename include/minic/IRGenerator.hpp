#ifndef MINIC_IR_GENERATOR_HPP
#define MINIC_IR_GENERATOR_HPP

#include "ASTVisitor.hpp"
#include "IR.hpp"
#include <map>
#include <memory>
#include <string>

namespace minic
{

/**
 * @class IRGenerator
 * @brief Generate an intermediate representation (IR) from the AST.
 *
 * IRGenerator traverses the parsed AST (via ASTVisitor) and emits a
 * linear IR represented by IRProgram / IRFunction / BasicBlock /
 * IRInstruction. It manages temporary names, current function and block
 * context, and a mapping from source variables to IR temporaries.
 */
class IRGenerator : public ASTVisitor
{
public:
    /**
     * @brief Generate an IRProgram for the given AST Program.
     * @param program The AST root program to translate.
     * @return A unique_ptr owning the generated IRProgram.
     */
    std::unique_ptr<IRProgram> generate(const Program& program);

    /**
     * @brief Visit the top-level Program node.
     * @param program The program AST node.
     *
     * Implements ASTVisitor::visit to traverse and translate program-level
     * declarations and functions into IRFunctions.
     */
    void visit(const Program& program) override;

    /**
     * @brief Visit a Function AST node and produce an IRFunction.
     * @param function The function AST node.
     *
     * Sets up function-level state (current_function_, blocks, parameter
     * mapping) and emits instructions for the function body.
     */
    void visit(const Function& function) override;

    /**
     * @brief Visit a statement AST node and emit corresponding IR.
     * @param stmt The statement AST node.
     *
     * Dispatches on statement kind (block, return, if, while, expr-stmt, etc.)
     * and emits one or more IR instructions/blocks as needed.
     */
    void visit(const Stmt& stmt) override;

    /**
     * @brief Visit an expression AST node and emit IR to compute its value.
     * @param expr The expression AST node.
     *
     * Expression visits typically produce temporaries and may emit multiple
     * instructions; the produced temporary is returned by generate_expr.
     */
    void visit(const Expr& expr) override;

private:
    std::unique_ptr<IRProgram> ir_program_; ///< Owned IRProgram under construction
    IRFunction* current_function_ = nullptr; ///< Function being generated (non-owning)
    BasicBlock* current_block_ = nullptr; ///< Current basic block for emitting instructions
    int temp_counter_ = 0; ///< Counter for generating unique temporaries
    std::map<std::string, std::string> var_map_; ///< Map from source var name to IR temp/slot

    /**
     * @brief Create a new unique temporary variable name.
     * @return A string containing the new temporary name.
     */
    std::string new_temp();

    /**
     * @brief Emit an IR instruction into the current basic block.
     * @param op The opcode to emit.
     * @param res Optional result/target name.
     * @param op1 Optional first operand.
     * @param op2 Optional second operand.
     *
     * This helper constructs an IRInstruction and appends it to the
     * current_block_->instructions. current_block_ must be non-null.
     */
    void emit(IROpcode op, const std::string& res = "", const std::string& op1 = "", const std::string& op2 = "");

    /**
     * @brief Generate IR for an expression and return the result temporary.
     * @param expr The expression AST node.
     * @return The name of the temporary holding the expression result.
     *
     * This helper encapsulates expression lowering logic and returns the
     * temporary (or variable name) that holds the computed value.
     */
    std::string generate_expr(const Expr& expr); // Returns result temp
};

} // namespace minic

#endif // MINIC_IR_GENERATOR_HPP