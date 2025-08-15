#ifndef MINI_C_TOKEN_HPP
#define MINI_C_TOKEN_HPP

#include <cstddef>
#include <string>
#include <variant>

/**
 * @namespace minic
 * @brief Contains core types and definitions for the miniC language tokenizer.
 */
namespace minic
{

/**
 * @enum TokenType
 * @brief Enumerates all possible token types in the miniC language.
 *
 * This includes keywords, identifiers, literals, operators, punctuation,
 * indentation/line control, and special tokens.
 */
enum class TokenType
{
    // Keywords
    KEYWORD_INT,
    KEYWORD_VOID,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_WHILE,
    KEYWORD_RETURN,

    // Identifiers
    IDENTIFIER,

    // Literals
    LITERAL_INT,
    LITERAL_STRING,

    // Operators
    OP_PLUS, // +
    OP_MINUS, // -
    OP_MULTIPLY, // *
    OP_DIVIDE, // /
    OP_ASSIGN, // =
    OP_EQUAL, // ==
    OP_NOT_EQUAL, // !=
    OP_LESS, // <
    OP_GREATER, // >
    OP_LESS_EQ, // <=
    OP_GREATER_EQ, // >=

    // Punctuation
    LPAREN, // (
    RPAREN, // )
    COLON, // :
    COMMA, // ,

    // Indentation and Line Control
    INDENT, // Start of a new block
    DEDENT, // End of a block
    NEWLINE, // Line break to separate statements

    // Special
    END_OF_FILE
};

/**
 * @struct Token
 * @brief Represents a single token produced by the miniC lexer.
 *
 * @var Token::type
 *   The type of the token, as defined by TokenType.
 * @var Token::value
 *   The value of the token. Holds either an integer (for integer literals)
 *   or a string (for identifiers and keywords).
 * @var Token::line
 *   The line number in the source code where the token was found.
 * @var Token::column
 *   The column number in the source code where the token starts.
 * @var Token::length
 *  The length of the token in characters, useful for error reporting and debugging.
 */
struct Token
{
    TokenType type;
    std::variant<int, std::string> value; // Holds int literal or string (for identifiers/keywords)
    size_t line;
    size_t column;
    size_t length; // Length of the token in characters
};

}

#endif // MINI_C_TOKEN_HPP
