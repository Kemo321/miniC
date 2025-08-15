#ifndef MINI_C_LEXER_HPP
#define MINI_C_LEXER_HPP
#include "Token.hpp"
#include <stack>
#include <string>
#include <variant>
#include <vector>

/**
 * @namespace minic
 * @brief Contains components for the miniC language, including the Lexer for tokenizing source code.
 */
namespace minic
{
/**
 * @class Lexer
 * @brief Tokenizes miniC source code into a sequence of tokens.
 *
 * The Lexer class reads a string containing miniC source code and produces a vector of Token objects.
 * It supports handling of indentation, comments, identifiers, numbers, strings, and enforces indentation consistency.
 */
class Lexer
{
public:
    /**
     * @brief Constructs a Lexer with the given source code.
     * @param source The miniC source code to tokenize.
     */
    explicit Lexer(const std::string& source);

    /**
     * @brief Tokenizes the entire input source code.
     * @return A vector of Token objects representing the tokenized source.
     */
    std::vector<minic::Token> Lex();

private:
    std::string source_;
    size_t pos_ = 0;
    size_t line_ = 1;
    size_t column_ = 1;
    std::stack<size_t> indent_levels_; // Track indentation levels (in spaces)

    /**
     * @brief Returns the current character without advancing the position.
     * @return The current character in the source.
     */
    char peek() const;

    /**
     * @brief Consumes and returns the current character, advancing the position.
     * @return The consumed character.
     */
    char advance();

    /**
     * @brief Checks if the lexer has reached the end of the input.
     * @return True if at end of input, false otherwise.
     */
    bool is_at_end() const;

    /**
     * @brief Retrieves the next token from the source code.
     * @return The next Token object.
     */
    Token next_token();

    /**
     * @brief Skips whitespace characters that are not indentation.
     */
    void skip_whitespace();

    /**
     * @brief Skips comments, including single-line (//) and multi-line (/* ...).
     */
    void skip_comment();

    /**
     * @brief Scans and returns an identifier or keyword token.
     * @return The scanned Token object.
     */
    Token scan_identifier();

    /**
     * @brief Scans and returns an integer literal token.
     * @return The scanned Token object.
     */
    Token scan_number();

    /**
     * @brief Scans and returns a string literal token.
     * @return The scanned Token object.
     */
    Token scan_string();

    /**
     * @brief Processes indentation at the start of a line and returns an indentation token.
     * @return The indentation Token object.
     */
    Token handle_indentation();

    /**
     * @brief Creates a Token object of the specified type and value.
     * @param type The type of token.
     * @param value The value of the token (optional).
     * @return The created Token object.
     */
    Token make_token(TokenType type, const std::variant<int, std::string>& value = {}) const;

    /**
     * @brief Ensures indentation consistency (no mixing of tabs and spaces).
     * @param c The current indentation character.
     */
    void check_indent_consistency(char c);
};

} // namespace minic

#endif // MINI_C_TOKEN_HPP