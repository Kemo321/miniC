#ifndef MINIC_AST_HPP
#define MINIC_AST_HPP
#include "miniC/Lexer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace minic
{

/**
 * @brief Base class for all AST nodes.
 */
class ASTNode
{
public:
    virtual ~ASTNode() = default;
};

/**
 * @brief Base class for all expression nodes.
 */
class Expr : public ASTNode
{
};

/**
 * @brief Integer literal expression.
 */
class IntLiteral : public Expr
{
public:
    int value;
    explicit IntLiteral(int val)
        : value(val)
    {
    }
};

/**
 * @brief String literal expression.
 */
class StringLiteral : public Expr
{
public:
    std::string value;
    explicit StringLiteral(const std::string& val)
        : value(val)
    {
    }
};

/**
 * @brief Identifier expression.
 */
class Identifier : public Expr
{
public:
    std::string name;
    explicit Identifier(const std::string& n)
        : name(n)
    {
    }
};

/**
 * @brief Binary expression (e.g., arithmetic or logical).
 */
class BinaryExpr : public Expr
{
public:
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
    TokenType op;
    BinaryExpr(std::unique_ptr<Expr> l, TokenType o, std::unique_ptr<Expr> r)
        : left(std::move(l))
        , right(std::move(r))
        , op(o)
    {
    }
};

/**
 * @brief Base class for all statement nodes.
 */
class Stmt : public ASTNode
{
};

/**
 * @brief Return statement.
 */
class ReturnStmt : public Stmt
{
public:
    std::unique_ptr<Expr> value;
    explicit ReturnStmt(std::unique_ptr<Expr> v)
        : value(std::move(v))
    {
    }
};

/**
 * @brief If statement with then and else branches.
 */
class IfStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> then_branch;
    std::vector<std::unique_ptr<Stmt>> else_branch;
    IfStmt(std::unique_ptr<Expr> cond,
        std::vector<std::unique_ptr<Stmt>> then_b,
        std::vector<std::unique_ptr<Stmt>> else_b)
        : condition(std::move(cond))
        , then_branch(std::move(then_b))
        , else_branch(std::move(else_b))
    {
    }
};

/**
 * @brief While loop statement.
 */
class WhileStmt : public Stmt
{
public:
    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Stmt>> body;
    WhileStmt(std::unique_ptr<Expr> cond, std::vector<std::unique_ptr<Stmt>> b)
        : condition(std::move(cond))
        , body(std::move(b))
    {
    }
};

/**
 * @brief Assignment statement.
 */
class AssignStmt : public Stmt
{
public:
    std::string name;
    std::unique_ptr<Expr> value;
    AssignStmt(const std::string& n, std::unique_ptr<Expr> v)
        : name(n)
        , value(std::move(v))
    {
    }
};

/**
 * @brief Function parameter declaration.
 */
struct Parameter
{
    TokenType type; ///< Parameter type (e.g., KEYWORD_INT, KEYWORD_VOID)
    std::string name; ///< Parameter name
    Parameter(TokenType t, const std::string& n)
        : type(t)
        , name(n)
    {
    }
};

/**
 * @brief Function definition.
 */
class Function : public ASTNode
{
public:
    std::string name;
    TokenType return_type;
    std::vector<Parameter> parameters;
    std::vector<std::unique_ptr<Stmt>> body;
    Function(const std::string& n,
        TokenType rt,
        std::vector<Parameter> params,
        std::vector<std::unique_ptr<Stmt>> b)
        : name(n)
        , return_type(rt)
        , parameters(std::move(params))
        , body(std::move(b))
    {
    }
};

/**
 * @brief Program root node containing all functions.
 */
class Program : public ASTNode
{
public:
    std::vector<std::unique_ptr<Function>> functions;
    explicit Program(std::vector<std::unique_ptr<Function>> f)
        : functions(std::move(f))
    {
    }
};

} // namespace minic

#endif // MINIC_AST_HPP