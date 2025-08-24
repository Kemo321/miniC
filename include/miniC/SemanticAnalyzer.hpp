#ifndef MINIC_SEMANTIC_ANALYZER_HPP
#define MINIC_SEMANTIC_ANALYZER_HPP

#include "ASTVisitor.hpp"
#include "ast.hpp"
#include <map>
#include <stdexcept>
#include <string>

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including tools for parsing, AST representation,
 * and semantic analysis of miniC programs.
 */
namespace minic
{

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
    ~SemanticAnalyzer();

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
    /**
     * @struct SymbolTable
     * @brief Simple mapping of variable names to their token/type information for the current scope.
     *
     * This table supports basic operations required by the analyzer:
     *  - Checking whether an identifier is declared.
     *  - Retrieving the type associated with a declared identifier.
     *
     * Note: get_type throws std::runtime_error if a lookup fails; callers should handle or translate
     * such errors into user-facing diagnostics as appropriate.
     */
    struct SymbolTable
    {
        std::map<std::string, TokenType> variables; ///< Maps variable names to their TokenType.

        /**
         * @brief Determines whether a variable name is present in the table.
         * @param name The variable identifier to check.
         * @return True if declared in this table, false otherwise.
         */
        bool is_declared(const std::string& name) const;

        /**
         * @brief Retrieves the type associated with a variable name.
         * @param name The variable identifier to look up.
         * @return The TokenType associated with name.
         * @throws std::runtime_error if the variable is not found.
         */
        TokenType get_type(const std::string& name) const;
    };

    SymbolTable current_scope_; ///< The symbol table representing the current (active) scope.
};

} // namespace minic

#endif // MINIC_SEMANTIC_ANALYZER_HPP