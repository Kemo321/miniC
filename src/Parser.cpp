#include "miniC/Parser.hpp"
#include <stdexcept>

namespace minic
{

Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens)
{
}

std::unique_ptr<Program> Parser::parse()
{
    std::vector<std::unique_ptr<Function>> functions;
    while (!is_at_end())
    {
        functions.push_back(parse_function());
    }
    return std::make_unique<Program>(std::move(functions));
}

bool Parser::is_at_end() const
{
    return current_ >= tokens_.size() || tokens_[current_].type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const
{
    return tokens_[current_];
}

const Token& Parser::previous() const
{
    return tokens_[current_ - 1];
}

const Token& Parser::advance()
{
    if (!is_at_end())
        ++current_;
    return previous();
}

bool Parser::check(TokenType type) const
{
    return !is_at_end() && peek().type == type;
}

const Token& Parser::consume(TokenType type, const std::string& error)
{
    if (check(type))
        return advance();
    throw std::runtime_error(error + " at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
}

std::unique_ptr<Expr> Parser::parse_expression()
{
    return parse_comparison();
}

std::unique_ptr<Expr> Parser::parse_comparison()
{
    auto expr = parse_term();
    while (check(TokenType::OP_EQUAL) || check(TokenType::OP_NOT_EQUAL) || check(TokenType::OP_LESS) || check(TokenType::OP_LESS_EQ) || check(TokenType::OP_GREATER) || check(TokenType::OP_GREATER_EQ))
    {
        Token op = advance();
        auto right = parse_term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_term()
{
    auto expr = parse_factor();
    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS))
    {
        Token op = advance();
        auto right = parse_factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_factor()
{
    auto expr = parse_primary();
    while (check(TokenType::OP_MULTIPLY) || check(TokenType::OP_DIVIDE))
    {
        Token op = advance();
        auto right = parse_primary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_primary()
{
    if (check(TokenType::LITERAL_INT))
    {
        Token token = advance();
        return std::make_unique<IntLiteral>(std::get<int>(token.value));
    }
    if (check(TokenType::LITERAL_STRING))
    {
        Token token = advance();
        return std::make_unique<StringLiteral>(std::get<std::string>(token.value));
    }
    if (check(TokenType::IDENTIFIER))
    {
        Token token = advance();
        return std::make_unique<Identifier>(std::get<std::string>(token.value));
    }
    throw std::runtime_error("Expected expression at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
}

std::unique_ptr<Stmt> Parser::parse_statement()
{
    if (check(TokenType::KEYWORD_IF))
        return parse_if_statement();
    if (check(TokenType::KEYWORD_WHILE))
        return parse_while_statement();
    if (check(TokenType::KEYWORD_RETURN))
        return parse_return_statement();
    if (check(TokenType::IDENTIFIER))
        return parse_assign_statement();
    throw std::runtime_error("Expected statement at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
}

std::unique_ptr<Stmt> Parser::parse_if_statement()
{
    consume(TokenType::KEYWORD_IF, "Expected 'if'");
    auto condition = parse_expression();
    consume(TokenType::LBRACE, "Expected '{' after if condition");
    auto then_branch = parse_block();
    std::vector<std::unique_ptr<Stmt>> else_branch;
    if (check(TokenType::KEYWORD_ELSE))
    {
        advance();
        consume(TokenType::LBRACE, "Expected '{' after else");
        else_branch = parse_block();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Stmt> Parser::parse_while_statement()
{
    consume(TokenType::KEYWORD_WHILE, "Expected 'while'");
    auto condition = parse_expression();
    consume(TokenType::LBRACE, "Expected '{' after while condition");
    auto body = parse_block();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

std::unique_ptr<Stmt> Parser::parse_return_statement()
{
    consume(TokenType::KEYWORD_RETURN, "Expected 'return'");
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::SEMICOLON))
    {
        value = parse_expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after return");
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<Stmt> Parser::parse_assign_statement()
{
    Token name = consume(TokenType::IDENTIFIER, "Expected identifier");
    consume(TokenType::OP_ASSIGN, "Expected '='");
    auto value = parse_expression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment");
    return std::make_unique<AssignStmt>(std::get<std::string>(name.value), std::move(value));
}

std::vector<std::unique_ptr<Stmt>> Parser::parse_block()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    consume(TokenType::LBRACE, "Expected '{'");
    while (!check(TokenType::RBRACE) && !is_at_end())
    {
        statements.push_back(parse_statement());
    }
    consume(TokenType::RBRACE, "Expected '}'");
    return statements;
}

std::vector<Parameter> Parser::parse_parameters()
{
    std::vector<Parameter> params;
    if (!check(TokenType::RPAREN))
    {
        do
        {
            Token type = consume(TokenType::KEYWORD_INT, "Expected 'int' or 'void' for parameter type");
            Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            params.emplace_back(type.type, std::get<std::string>(name.value));
        } while (check(TokenType::COMMA) && (advance(), true));
    }
    return params;
}

std::unique_ptr<Function> Parser::parse_function()
{
    Token type = consume(TokenType::KEYWORD_INT, "Expected 'int' or 'void'");
    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '('");
    auto parameters = parse_parameters();
    consume(TokenType::RPAREN, "Expected ')'");
    consume(TokenType::LBRACE, "Expected '{'");
    auto body = parse_block();
    return std::make_unique<Function>(std::get<std::string>(name.value), type.type, std::move(parameters), std::move(body));
}

void Parser::synchronize()
{
    while (!is_at_end())
    {
        if (check(TokenType::SEMICOLON))
        {
            advance();
            break;
        }
        advance();
    }
}

} // namespace minic