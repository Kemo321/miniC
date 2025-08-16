#pragma once
#include "miniC/Lexer.hpp"
#include "miniC/ast.hpp"
#include <vector>

namespace minic
{

class Parser
{
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens_;
    size_t current_ = 0;

    bool is_at_end() const;
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool check(TokenType type) const;
    const Token& consume(TokenType type, const std::string& error);
    void synchronize();

    std::unique_ptr<Expr> parse_expression();
    std::unique_ptr<Expr> parse_comparison();
    std::unique_ptr<Expr> parse_term();
    std::unique_ptr<Expr> parse_factor();
    std::unique_ptr<Expr> parse_primary();
    std::unique_ptr<Stmt> parse_statement();
    std::unique_ptr<Stmt> parse_if_statement();
    std::unique_ptr<Stmt> parse_while_statement();
    std::unique_ptr<Stmt> parse_return_statement();
    std::unique_ptr<Stmt> parse_assign_statement();
    std::vector<std::unique_ptr<Stmt>> parse_block();
    std::vector<Parameter> parse_parameters(); // Added for parameters
    std::unique_ptr<Function> parse_function();
};

} // namespace minic