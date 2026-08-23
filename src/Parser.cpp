#include "minic/Parser.hpp"
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
    if (!tokens_.empty())
    {
        return current_ >= tokens_.size() || tokens_[current_].type == TokenType::END_OF_FILE;
    }
    return true;
}

const Token& Parser::peek() const
{
    if (!is_at_end())
        return tokens_[current_];
    throw std::runtime_error("No current token");
}

const Token& Parser::previous() const
{
    if (current_ > 0)
        return tokens_[current_ - 1];
    throw std::runtime_error("No previous token");
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
    auto expr = parse_unary();
    while (check(TokenType::OP_MULTIPLY) || check(TokenType::OP_DIVIDE))
    {
        Token op = advance();
        auto right = parse_unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op.type, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parse_unary()
{
    if (check(TokenType::OP_NOT) || check(TokenType::OP_MINUS))
    {
        Token op = advance();
        auto operand = parse_unary();
        return std::make_unique<UnaryExpr>(op.type, std::move(operand));
    }
    if (check(TokenType::OP_MULTIPLY))
    {
        advance();
        auto operand = parse_unary();
        return std::make_unique<DereferenceExpr>(std::move(operand));
    }
    if (check(TokenType::OP_ADDRESS))
    {
        advance();
        auto operand = parse_unary();
        return std::make_unique<AddressOfExpr>(std::move(operand));
    }
    return parse_primary();
}

std::unique_ptr<Expr> Parser::parse_primary()
{
    // Parenthesized expression
    if (check(TokenType::LPAREN))
    {
        advance();
        auto expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }

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
        std::string name = std::get<std::string>(token.value);

        // Function call: name(arg1, arg2, ...)
        if (check(TokenType::LPAREN))
        {
            advance(); // consume '('
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN))
            {
                do
                {
                    args.push_back(parse_expression());
                } while (check(TokenType::COMMA) && (advance(), true));
            }
            consume(TokenType::RPAREN, "Expected ')' after function arguments");
            return std::make_unique<CallExpr>(name, std::move(args));
        }

        return std::make_unique<Identifier>(name);
    }
    throw std::runtime_error("Expected expression at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
}

TokenType Parser::parse_type()
{
    Token type;
    if (check(TokenType::KEYWORD_INT))
        type = advance();
    else if (check(TokenType::KEYWORD_VOID))
        type = advance();
    else if (check(TokenType::KEYWORD_STR))
        type = advance();
    else
        throw std::runtime_error("Expected type (int, void, string) at line " + std::to_string(peek().line));

    if (check(TokenType::OP_MULTIPLY))
    {
        advance();
        if (type.type != TokenType::KEYWORD_INT)
            throw std::runtime_error("Only 'int *' pointers are supported at line " + std::to_string(type.line));
        return TokenType::TYPE_PTR_INT;
    }
    return type.type;
}

std::unique_ptr<Stmt> Parser::parse_statement()
{
    if (check(TokenType::KEYWORD_IF))
        return parse_if_statement();
    if (check(TokenType::KEYWORD_WHILE))
        return parse_while_statement();
    if (check(TokenType::KEYWORD_RETURN))
        return parse_return_statement();
    if (check(TokenType::KEYWORD_BREAK))
    {
        advance();
        consume(TokenType::SEMICOLON, "Expected ';' after break");
        return std::make_unique<BreakStmt>();
    }
    if (check(TokenType::KEYWORD_CONTINUE))
    {
        advance();
        consume(TokenType::SEMICOLON, "Expected ';' after continue");
        return std::make_unique<ContinueStmt>();
    }
    if (check(TokenType::KEYWORD_INT) || check(TokenType::KEYWORD_VOID) || check(TokenType::KEYWORD_STR))
    {
        return parse_var_decl_statement();
    }
    if (check(TokenType::IDENTIFIER))
    {
        // Distinguish assignment (id = ...) from call / other expression statements (id(...);)
        if (current_ + 1 < tokens_.size() && tokens_[current_ + 1].type == TokenType::OP_ASSIGN)
            return parse_assign_statement();

        auto expr = parse_expression();
        consume(TokenType::SEMICOLON, "Expected ';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    // Expression statements starting with unary ops / literals / '(' — including *p = ...
    if (check(TokenType::OP_MULTIPLY) || check(TokenType::OP_ADDRESS) || check(TokenType::OP_NOT)
        || check(TokenType::OP_MINUS) || check(TokenType::LPAREN) || check(TokenType::LITERAL_INT)
        || check(TokenType::LITERAL_STRING))
    {
        auto expr = parse_expression();
        if (check(TokenType::OP_ASSIGN))
        {
            auto* deref = dynamic_cast<DereferenceExpr*>(expr.get());
            if (!deref)
            {
                throw std::runtime_error("Invalid assignment target at line " + std::to_string(peek().line));
            }
            std::unique_ptr<Expr> target = std::move(deref->operand);
            expr.reset();
            advance(); // '='
            auto value = parse_expression();
            consume(TokenType::SEMICOLON, "Expected ';' after assignment");
            return std::make_unique<DerefAssignStmt>(std::move(target), std::move(value));
        }
        consume(TokenType::SEMICOLON, "Expected ';' after expression");
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    throw std::runtime_error("Expected statement at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
}

std::unique_ptr<Stmt> Parser::parse_if_statement()
{
    consume(TokenType::KEYWORD_IF, "Expected 'if'");
    if (check(TokenType::LPAREN))
        advance();
    auto condition = parse_expression();
    if (check(TokenType::RPAREN))
        advance();
    auto then_branch = parse_block();
    std::vector<std::unique_ptr<Stmt>> else_branch;
    if (check(TokenType::KEYWORD_ELSE))
    {
        advance();
        else_branch = parse_block();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

std::unique_ptr<Stmt> Parser::parse_while_statement()
{
    consume(TokenType::KEYWORD_WHILE, "Expected 'while'");
    if (check(TokenType::LPAREN))
        advance();
    auto condition = parse_expression();
    if (check(TokenType::RPAREN))
        advance();
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

std::unique_ptr<Stmt> Parser::parse_var_decl_statement()
{
    TokenType decl_type = parse_type();
    if (decl_type == TokenType::KEYWORD_VOID)
    {
        throw std::runtime_error("Cannot declare a void variable at line " + std::to_string(peek().line));
    }
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name");
    std::unique_ptr<Expr> initializer = nullptr;
    if (check(TokenType::OP_ASSIGN))
    {
        advance();
        initializer = parse_expression();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after declaration");
    return std::make_unique<VarDeclStmt>(decl_type, std::get<std::string>(name.value), std::move(initializer));
}

std::vector<std::unique_ptr<Stmt>> Parser::parse_block()
{
    std::vector<std::unique_ptr<Stmt>> statements;
    while (check(TokenType::NEWLINE))
        advance(); // Skip newlines
    consume(TokenType::LBRACE, "Expected '{'");
    while (check(TokenType::NEWLINE))
        advance(); // Skip newlines
    while (!check(TokenType::RBRACE) && !is_at_end())
    {
        statements.push_back(parse_statement());
        while (check(TokenType::NEWLINE))
            advance();
    }
    while (check(TokenType::NEWLINE))
        advance();
    consume(TokenType::RBRACE, "Expected '}'");
    while ((check(TokenType::NEWLINE)))
        advance();
    return statements;
}

std::vector<Parameter> Parser::parse_parameters()
{
    std::vector<Parameter> params;
    if (!check(TokenType::RPAREN))
    {
        do
        {
            TokenType param_type = parse_type();
            Token name = consume(TokenType::IDENTIFIER, "Expected parameter name");
            params.emplace_back(param_type, std::get<std::string>(name.value));
        } while (check(TokenType::COMMA) && (advance(), true));
    }
    return params;
}

std::unique_ptr<Function> Parser::parse_function()
{
    while (check(TokenType::NEWLINE))
        advance(); // Skip newlines

    Token type;
    if (check(TokenType::KEYWORD_INT))
    {
        type = consume(TokenType::KEYWORD_INT, "Expected 'int' or 'void'");
    }
    else if (check(TokenType::KEYWORD_VOID))
    {
        type = consume(TokenType::KEYWORD_VOID, "Expected 'int' or 'void'");
    }
    else if (check(TokenType::KEYWORD_STR))
    {
        type = consume(TokenType::KEYWORD_STR, "Expected 'int', 'void' or 'str'");
    }
    else
    {
        throw std::runtime_error("Expected 'int', 'void' or 'str' for function return type at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
    }

    Token name = consume(TokenType::IDENTIFIER, "Expected function name");
    consume(TokenType::LPAREN, "Expected '('");
    auto parameters = parse_parameters();
    consume(TokenType::RPAREN, "Expected ')'");
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