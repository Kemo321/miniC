#ifndef MINIC_IR_GENERATOR_HPP
#define MINIC_IR_GENERATOR_HPP

#include "minic/ASTVisitor.hpp"
#include "minic/IR.hpp"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace minic
{

/**
 * @class IRGenerator
 * @brief Generate Intermediate Representation (IR) from the AST.
 *
 * IRGenerator is an ASTVisitor that traverses the parsed AST and emits a
 * sequence of IRFunction and BasicBlock objects inside an IRProgram.
 * It maintains generation state such as the current function and block,
 * temporary and label counters, and a mapping from source variable names to
 * IR temporaries/variables.
 */
class IRGenerator : public ASTVisitor
{
public:
    /**
     * @brief Generate an IRProgram for the given AST program.
     *
     * Traverses the top-level Program node and produces a unique_ptr to an
     * IRProgram containing one IRFunction per source function and the
     * corresponding basic blocks and instructions.
     *
     * @param program AST root Program node.
     * @return Owned IRProgram representing the compiled IR.
     */
    std::unique_ptr<IRProgram> generate(const Program& program);

    /**
     * @brief Visit a Program node.
     *
     * Called during traversal of the AST's root program; responsible for
     * visiting contained functions and populating the IRProgram.
     */
    void visit(const Program& program) override;

    /**
     * @brief Visit a Function node.
     *
     * Creates a corresponding IRFunction, initializes entry BasicBlock(s),
     * and emits IR for the function body.
     */
    void visit(const Function& function) override;

    /**
     * @brief Visit a statement node.
     *
     * Emits IR instructions or control-flow modifications corresponding to
     * the given statement.
     */
    void visit(const Stmt& stmt) override;

    /**
     * @brief Visit an expression node.
     *
     * Evaluates the expression into IR by emitting necessary instructions and
     * returning/recording temporaries via generate_expr.
     */
    void visit(const Expr& expr) override;

private:
    std::unique_ptr<IRProgram> ir_program_; ///< Owned IRProgram being built
    IRFunction* current_function_ = nullptr; ///< Currently emitting function (non-owning)
    BasicBlock* current_block_ = nullptr; ///< Currently emitting basic block (non-owning)
    int temp_counter_ = 0; ///< Counter to generate unique temporary names
    int label_counter_ = 0; ///< Counter to generate unique labels
    std::map<std::string, std::string> var_map_; ///< Map from source var name to IR var/temp

    /**
     * @brief Create a fresh temporary variable name.
     *
     * Returns a unique temporary string (used as result names for instructions).
     */
    std::string new_temp();

    /**
     * @brief Create a fresh label with the given prefix.
     *
     * Useful for generating basic block labels or branch targets.
     *
     * @param prefix Label prefix to make generated labels more readable.
     */
    std::string new_label(const std::string& prefix);

    /**
     * @brief Emit an IR instruction into the current basic block.
     *
     * Convenience helper to append an IRInstruction with the supplied opcode
     * and operands to the current_block_. If no current block is set, this
     * function should handle that error state appropriately.
     *
     * @param op Opcode for the instruction.
     * @param res Optional result destination (temp/variable/label).
     * @param op1 Optional first operand.
     * @param op2 Optional second operand.
     */
    void emit(IROpcode op, const std::string& res = "", const std::string& op1 = "", const std::string& op2 = "");

    /**
     * @brief Generate IR for an expression and return its result name.
     *
     * Traverses the expression subtree, emits instructions to compute its
     * value, and returns the name of the temporary or variable holding the
     * computed value.
     *
     * @param expr Expression AST node to translate.
     * @return Name of the IR temporary or variable that contains the result.
     */
    std::string generate_expr(const Expr& expr); // Returns result temp/var

    friend class PublicIRGenerator; // Allow testing class to access private members
};

} // namespace minic

#endif // MINIC_IR_GENERATOR_HPP