#include "minic/Lexer.hpp"
#include <iostream>
#include <stdexcept>

namespace minic
{

Lexer::Lexer(const std::string& source)
    : source_(source)
    , pos_(0)
    , line_(1)
    , column_(1)
{
    // nothing to do here for now
}

std::vector<minic::Token> Lexer::Lex()
{
    std::vector<minic::Token> tokens;
    while (!is_at_end())
    {
        Token token = next_token();
        if (token.type == TokenType::END_OF_FILE)
            break; // Stop processing at end of file
        tokens.push_back(token);
    }
    // Add an end of file token
    tokens.push_back(make_token(TokenType::END_OF_FILE, {}));
    return tokens;
}

char Lexer::peek() const
{
    if (pos_ < source_.size())
    {
        return source_[pos_];
    }
    // Return null character if at end of input
    return '\0';
}

char Lexer::peek_next() const
{
    if (pos_ + 1 < source_.size())
    {
        return source_[pos_ + 1];
    }
    // Return null character if at end of input
    return '\0';
}

char Lexer::advance()
{
    if (pos_ < source_.size())
    {
        char current = source_[pos_++];
        if (current == '\n')
        {
            line_++;
            column_ = 1;
        }
        else
        {
            column_++;
        }
        return current;
    }
    return '\0';
}

bool Lexer::is_at_end() const
{
    return pos_ >= source_.size();
}

Token Lexer::next_token()
{
    skip_whitespace();
    if (is_at_end())
        return make_token(TokenType::END_OF_FILE);

    char current = peek();

    // Handle comments
    if (current == '/' && pos_ + 1 < source_.size())
    {
        char next = source_[pos_ + 1];
        if (next == '/' || next == '*')
        {
            skip_comment();
            return next_token();
        }
    }

    // Handle literals and identifiers
    if (std::isdigit(current))
        return scan_number();

    if (std::isalpha(current) || current == '_')
        return scan_identifier();

    if (current == '"')
        return scan_string();

    // Handle single-character tokens
    switch (current)
    {
    case '{':
    {
        Token t = make_token(TokenType::LBRACE);
        advance();
        return t;
    }
    case '}':
    {
        Token t = make_token(TokenType::RBRACE);
        advance();
        return t;
    }
    case ';':
    {
        Token t = make_token(TokenType::SEMICOLON);
        advance();
        return t;
    }
    case '(':
    {
        Token t = make_token(TokenType::LPAREN);
        advance();
        return t;
    }
    case ')':
    {
        Token t = make_token(TokenType::RPAREN);
        advance();
        return t;
    }
    case '\n':
    {
        Token t = make_token(TokenType::NEWLINE);
        advance();
        return t;
    }
    case ' ':
    case '\t':
    case '\r':
        advance();
        return next_token();
    case '+':
    {
        Token t = make_token(TokenType::OP_PLUS);
        advance();
        return t;
    }
    case '-':
    {
        Token t = make_token(TokenType::OP_MINUS);
        advance();
        return t;
    }
    case '*':
    {
        Token t = make_token(TokenType::OP_MULTIPLY);
        advance();
        return t;
    }
    case '/':
    {
        Token t = make_token(TokenType::OP_DIVIDE);
        advance();
        return t;
    }
    case '<':
    {
        if (peek_next() == '=')
        {
            Token t = make_token(TokenType::OP_LESS_EQ);
            advance();
            advance();
            return t;
        }
        else
        {
            Token t = make_token(TokenType::OP_LESS);
            advance();
            return t;
        }
    }
    case '>':
    {
        if (peek_next() == '=')
        {
            Token t = make_token(TokenType::OP_GREATER_EQ);
            advance();
            advance();
            return t;
        }
        else
        {
            Token t = make_token(TokenType::OP_GREATER);
            advance();
            return t;
        }
    }
    case ':':
    {
        Token t = make_token(TokenType::COLON);
        advance();
        return t;
    }
    case ',':
    {
        Token t = make_token(TokenType::COMMA);
        advance();
        return t;
    }
    case '!':
    {
        if (peek_next() == '=')
        {
            Token t = make_token(TokenType::OP_NOT_EQUAL);
            advance();
            advance();
            return t;
        }
        else
        {
            Token t = make_token(TokenType::OP_NOT);
            advance();
            return t;
        }
    }
    case '=':
        if (pos_ + 1 < source_.size() && source_[pos_ + 1] == '=')
        {
            Token t = make_token(TokenType::OP_EQUAL);
            advance();
            advance();
            return t;
        }
        else
        {
            Token t = make_token(TokenType::OP_ASSIGN);
            advance();
            return t;
        }
    default:
        throw std::runtime_error("Unexpected character: " + std::string(1, current));
    }
}

void Lexer::skip_whitespace()
{
    while (peek() == ' ' || peek() == '\t' || peek() == '\r')
    {
        advance();
    }
}

void Lexer::skip_comment()
{
    if (peek() == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/')
    {
        // Single-line comment
        while (peek() != '\n' && !is_at_end())
        {
            advance();
        }
    }
    else if (peek() == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*')
    {
        // Multi-line comment
        advance(); // Skip '/'
        advance(); // Skip '*'
        while (!(peek() == '*' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') && !is_at_end())
        {
            advance();
        }
        if (!is_at_end())
        {
            advance(); // Skip '*'
            advance(); // Skip '/'
        }
    }
}

Token Lexer::scan_identifier()
{
    std::string identifier;
    Token token = make_token(TokenType::IDENTIFIER);
    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_' || peek() == '$')) // Allow alphanumeric and underscore
    {
        identifier += advance();
    }

    // Check if the identifier is a keyword
    TokenType type = TokenType::IDENTIFIER; // Default to IDENTIFIER
    if (identifier == "int")
        type = TokenType::KEYWORD_INT;
    else if (identifier == "void")
        type = TokenType::KEYWORD_VOID;
    else if (identifier == "if")
        type = TokenType::KEYWORD_IF;
    else if (identifier == "else")
        type = TokenType::KEYWORD_ELSE;
    else if (identifier == "while")
        type = TokenType::KEYWORD_WHILE;
    else if (identifier == "return")
        type = TokenType::KEYWORD_RETURN;
    else if (identifier == "string")
        type = TokenType::KEYWORD_STR;

    if (type == TokenType::IDENTIFIER)
    {
        token.value = identifier; // Store the identifier as a string
    }

    token.type = type;
    return token;
}

Token Lexer::scan_number()
{
    std::string num;
    Token token = make_token(TokenType::LITERAL_INT);
    while (!is_at_end() && std::isdigit(static_cast<unsigned char>(peek())))
    {
        num += advance();
    }
    try
    {
        token.value = std::stoi(num);
    }
    catch (const std::invalid_argument&)
    {
        throw std::runtime_error("Invalid number literal at line " + std::to_string(line_) + ", column " + std::to_string(column_));
    }
    return token;
}

Token Lexer::scan_string()
{
    std::string str;

    // Capture start position (position of the opening quote)
    int start_line = line_;
    int start_column = column_;

    // Skip opening quote (this will advance column_ / pos_)
    advance();

    // Create token and set its start position explicitly so it points to the opening quote.
    Token token = make_token(TokenType::LITERAL_STRING);
    token.line = start_line;
    token.column = start_column;

    while (!is_at_end())
    {
        char c = advance();

        // Closing quote -> finish
        if (c == '"')
        {
            token.value = str;
            return token;
        }

        // Handle escape sequences: consume the next raw source char(s), decode into str,
        if (c == '\\')
        {
            if (is_at_end())
            {
                throw std::runtime_error("Unterminated escape sequence starting at line " + std::to_string(start_line) + ", column " + std::to_string(start_column));
            }

            char esc = advance(); // consumes the escape char in the source

            switch (esc)
            {
            case 'n':
                str.push_back('\n');
                break;
            case 't':
                str.push_back('\t');
                break;
            case 'r':
                str.push_back('\r');
                break;
            case 'b':
                str.push_back('\b');
                break;
            case '"':
                str.push_back('"');
                break;
            case '\\':
                str.push_back('\\');
                break;
            default:
                // Report the position of the escape char. column_ currently points after it,
                // so column_-1 is where the esc char sits in source coordinates.
                {
                    int col_pos = static_cast<int>(column_) - 1;
                    if (col_pos < 1)
                        col_pos = 1;
                    throw std::runtime_error(std::string("Unknown escape sequence \\") + esc + " at line " + std::to_string(line_) + ", column " + std::to_string(col_pos));
                }
            }
        }
        else
        {
            str.push_back(c);
        }
    }

    // If we fell out, the closing quote was missing
    throw std::runtime_error("Unclosed string literal starting at line " + std::to_string(start_line) + ", column " + std::to_string(start_column));
}

Token Lexer::make_token(TokenType type, const std::variant<int, std::string>& value) const
{
    return Token { type, value, line_, column_ };
}

} // namespace minic
