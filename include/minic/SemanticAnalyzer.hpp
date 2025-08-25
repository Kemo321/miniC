#ifndef MINIC_SEMANTIC_ANALYZER_HPP
#define MINIC_SEMANTIC_ANALYZER_HPP

#include "ASTVisitor.hpp"
#include "minic/AST.hpp"
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including tools for parsing, AST representation,
 * and semantic analysis of miniC programs.
 */
namespace minic
{

/**
 * @class SemanticError
 * @brief Custom exception for semantic errors, allowing for better error reporting.
 */
class SemanticError : public std::runtime_error
{
public:
    explicit SemanticError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

/**
 * @class SemanticAnalyzer
 * @brief Performs semantic analysis by traversing the AST and validating program correctness.
 *
 * The SemanticAnalyzer implements the ASTVisitor interface to walk a Program AST and enforce
 * semantic rules such as:
 *  - Declaration and scoping rules for variables and functions.
 *  - Type consistency for expressions, assignments, and return statements.
 *  - Validation of function definitions and their bodies.
 *  - Detection and reporting of common semantic errors (e.g., use of undeclared identifiers,
 *    mismatched types).
 *
 * The analyzer uses an internal SymbolTable to track variables in the current scope and provides
 * helper routines to check functions, statements, and expressions. Errors may be reported via
 * exceptions or a diagnostic mechanism (implementation-defined).
 */
class SemanticAnalyzer : public ASTVisitor
{
public:
    /**
     * @brief Default constructs a SemanticAnalyzer.
     *
     * Prepares an analyzer ready to visit an AST. No external resources are required by default.
     */
    SemanticAnalyzer();

    /**
     * @brief Destructor.
     *
     * Cleans up any analyzer resources. Override is provided to satisfy polymorphic base class behavior.
     */
    ~SemanticAnalyzer() override;

    /**
     * @brief Visits the Program AST node and performs top-level semantic checks.
     *
     * Entry point for analyzing an entire program: this typically involves iterating over
     * function definitions and global declarations and validating program-wide constraints.
     *
     * @param program The Program node to analyze.
     */
    void visit(const Program& program) override;

    /**
     * @brief Visits a Function AST node to validate its signature and body.
     *
     * Ensures parameters are well-formed, the function body respects scoping rules, and return
     * types match declared types. May create a new scope for the function body.
     *
     * @param function The Function node being visited.
     */
    void visit(const Function& function) override;

    /**
     * @brief Visits a Stmt AST node and verifies statement-level semantics.
     *
     * Handles checks for declarations, assignments, control flow constructs, and block scoping.
     *
     * @param stmt The statement node to validate.
     */
    void visit(const Stmt& stmt) override;

    /**
     * @brief Visits an Expr AST node and performs type checking and expression validation.
     *
     * Ensures operands are compatible with operators, identifiers are declared, and infers or
     * verifies expression types as needed.
     *
     * @param expr The expression node to validate.
     */
    void visit(const Expr& expr) override;

private:
    using SymbolTable = std::unordered_map<std::string, TokenType>; ///< Maps variable names to their TokenType.

    std::stack<SymbolTable> scopes_; ///< Stack of symbol tables for nested scopes.
    std::unordered_map<std::string, TokenType> functions_; ///< Global function table (name to return type).

    TokenType current_function_type_ = TokenType::KEYWORD_VOID; ///< Track current function's return type.

    /**
     * @brief Pushes a new scope onto the stack.
     */
    void push_scope();

    /**
     * @brief Pops the current scope from the stack.
     */
    void pop_scope();

    /**
     * @brief Symbol table operations
     */
    bool is_declared_in_current_scope(const std::string& name) const;

    /**
     * @brief Checks if a variable is declared in any scope.
     * @param name The variable name to check.
     * @return True if declared, false otherwise.
     */
    bool is_declared(const std::string& name) const;

    /**
     * @brief Gets the type of a variable from the symbol table.
     * @param name The variable name to look up.
     * @return The variable's TokenType, or TokenType::UNKNOWN if not found.
     */
    TokenType get_type(const std::string& name) const;

    /**
     * @brief Infers the type of an expression.
     * @param expr The expression to analyze.
     * @return The inferred TokenType.
     */
    TokenType infer_type(const Expr& expr);

    /**
     * @brief Validates binary operator compatibility.
     * @param op The binary operator token.
     * @param left_type The inferred type of the left operand.
     * @param right_type The inferred type of the right operand.
     */
    void validate_binary_op(TokenType op, TokenType left_type, TokenType right_type);
};

} // namespace minic

#endif // MINIC_SEMANTIC_ANALYZER_HPP