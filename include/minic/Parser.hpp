#pragma once
#include "AST.hpp"

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including the Parser for building ASTs from tokens.
 */
namespace minic
{

/**
 * @class Parser
 * @brief Parses a sequence of tokens produced by the Lexer into an abstract syntax tree (AST).
 *
 * The Parser consumes a vector of Token objects and produces a Program AST representing the parsed
 * source program. It implements a recursive descent parsing strategy with methods for expressions,
 * statements, control flow constructs, function definitions, and blocks.
 */
class Parser
{
public:
    /**
     * @brief Constructs a Parser with the given token stream.
     * @param tokens A reference to a vector of Token objects produced by the Lexer.
     */
    Parser(const std::vector<Token>& tokens);

    /**
     * @brief Parses the entire token stream and returns a Program AST.
     * @return A unique_ptr to the parsed Program. May be null on failure.
     */
    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens_; ///< Reference to the token stream to parse.
    size_t current_ = 0; ///< Current index into tokens_.

    /**
     * @brief Checks whether the parser has reached the end of the token stream.
     * @return True if at end, false otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Returns the current token without consuming it.
     * @return A const reference to the current Token.
     */
    const Token& peek() const;

    /**
     * @brief Returns the most recently consumed token.
     * @return A const reference to the previous Token.
     */
    const Token& previous() const;

    /**
     * @brief Advances to the next token and returns the consumed token.
     * @return A const reference to the advanced-over Token.
     */
    const Token& advance();

    /**
     * @brief Checks whether the current token is of the specified type.
     * @param type The TokenType to check for.
     * @return True if the current token matches type, false otherwise.
     */
    bool check(TokenType type) const;

    /**
     * @brief Consumes a token of the expected type or reports an error.
     * @param type The expected TokenType.
     * @param error Error message used for reporting when the token does not match.
     * @return A const reference to the consumed Token.
     */
    const Token& consume(TokenType type, const std::string& error);

    /**
     * @brief Performs error recovery by discarding tokens until a likely statement boundary.
     *
     * This method is used after encountering a parse error to synchronize the parser state so
     * subsequent parsing can continue.
     */
    void synchronize();

    /**
     * @brief Parses an expression.
     * @return A unique_ptr to the parsed Expr node.
     */
    std::unique_ptr<Expr> parse_expression();

    /**
     * @brief Parses a comparison expression (e.g., <, >, ==).
     * @return A unique_ptr to the parsed Expr node.
     */
    std::unique_ptr<Expr> parse_comparison();

    /**
     * @brief Parses an additive expression (e.g., +, -).
     * @return A unique_ptr to the parsed Expr node.
     */
    std::unique_ptr<Expr> parse_term();

    /**
     * @brief Parses a multiplicative expression (e.g., *, /).
     * @return A unique_ptr to the parsed Expr node.
     */
    std::unique_ptr<Expr> parse_factor();

    /**
     * @brief Parses a primary expression (literals, identifiers, parenthesized expressions).
     * @return A unique_ptr to the parsed Expr node.
     */
    std::unique_ptr<Expr> parse_primary();

    /**
     * @brief Parses a statement (declaration, block, control flow, expression statement).
     * @return A unique_ptr to the parsed Stmt node.
     */
    std::unique_ptr<Stmt> parse_statement();

    /**
     * @brief Parses an if statement, including optional else branch.
     * @return A unique_ptr to the parsed Stmt node representing the if.
     */
    std::unique_ptr<Stmt> parse_if_statement();

    /**
     * @brief Parses a while loop statement.
     * @return A unique_ptr to the parsed Stmt node representing the while loop.
     */
    std::unique_ptr<Stmt> parse_while_statement();

    /**
     * @brief Parses a return statement.
     * @return A unique_ptr to the parsed Stmt node representing the return.
     */
    std::unique_ptr<Stmt> parse_return_statement();

    /**
     * @brief Parses an assignment statement.
     * @return A unique_ptr to the parsed Stmt node representing the assignment.
     */
    std::unique_ptr<Stmt> parse_assign_statement();

    /**
     * @brief Parses a variable declaration statement.
     * @return A unique_ptr to the parsed Stmt node representing the variable declaration.
     */
    std::unique_ptr<Stmt> parse_var_decl_statement();

    /**
     * @brief Parses a block of statements enclosed in braces.
     * @return A vector of unique_ptr<Stmt> representing the statements in the block.
     */
    std::vector<std::unique_ptr<Stmt>> parse_block();

    /**
     * @brief Parses a comma-separated parameter list for function definitions.
     * @return A vector of Parameter objects parsed from the token stream.
     */
    std::vector<Parameter> parse_parameters();

    /**
     * @brief Parses a function definition, including name, parameters, and body.
     * @return A unique_ptr to the parsed Function node.
     */
    std::unique_ptr<Function> parse_function();

    friend class PublicParser; ///< Exposes internals for testing or controlled external access.
};

} // namespace minic