#ifndef MINIC_AST_VISITOR_HPP
#define MINIC_AST_VISITOR_HPP
#include "miniC/ast.hpp"

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including AST node types and visitors.
 */
namespace minic
{

/**
 * @class ASTVisitor
 * @brief Visitor interface for traversing AST nodes of the miniC language.
 *
 * Implementations of this interface perform operations on AST nodes by overriding the visit
 * methods for Program, Function, Stmt, and Expr nodes. The visitor uses const references to
 * the nodes to avoid ownership transfers; implementations may cast to specific derived node
 * types as needed.
 */
class ASTVisitor
{
public:
    /**
     * @brief Virtual destructor.
     *
     * Ensures derived visitors are properly destroyed through base pointers.
     */
    virtual ~ASTVisitor() = default;

    /**
     * @brief Visit the whole program.
     * @param program The Program AST node to visit.
     */
    virtual void visit(const Program& program) = 0;

    /**
     * @brief Visit a function declaration/definition.
     * @param function The Function AST node to visit.
     */
    virtual void visit(const Function& function) = 0;

    /**
     * @brief Visit a statement node.
     * @param stmt The Stmt AST node to visit.
     */
    virtual void visit(const Stmt& stmt) = 0;

    /**
     * @brief Visit an expression node.
     * @param expr The Expr AST node to visit.
     */
    virtual void visit(const Expr& expr) = 0;
};

} // namespace minic

#endif // MINIC_AST_VISITOR_HPP