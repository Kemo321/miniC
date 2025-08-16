#include "miniC/Lexer.hpp"
#include <deque> // added for pending dedent queue
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
    indent_levels_.push(0); // Start with an initial indentation level of 0
}

std::vector<minic::Token> Lexer::Lex()
{
    std::vector<minic::Token> tokens;
    while (!is_at_end())
    {
        Token token = next_token();
        if (token.type == TokenType::END_OF_FILE)
            break; // Stop processing at end of file
        if (token.type == TokenType::INDENT || token.type == TokenType::DEDENT)
        {
            tokens.push_back(Token { TokenType::NEWLINE, {}, line_ - 1, column_ - 1 });
        }
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
        Token token = handle_indentation();
        return token; // Handle indentation and newlines
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
        Token t = make_token(TokenType::OP_LESS);
        advance();
        return t;
    }
    case '>':
    {
        Token t = make_token(TokenType::OP_GREATER);
        advance();
        return t;
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
    while (!is_at_end() && std::isdigit(peek()))
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
    advance(); // Skip opening quote
    Token token = make_token(TokenType::LITERAL_STRING);
    while (!is_at_end() && peek() != '"')
    {
        str += advance();
    }
    if (!is_at_end())
    {
        advance(); // Skip closing quote
        token.value = str;
        return token;
    }
    // Handle error: unclosed string literal
    throw std::runtime_error("Unclosed string literal at line " + std::to_string(line_) + ", column " + std::to_string(column_));
}

Token Lexer::handle_indentation()
{
    // A small FIFO to hold extra DEDENT tokens so they can be returned across
    // repeated calls to next_token().
    static std::deque<Token> pending_dedents;

    // If we already prepared dedents from a previous newline, emit them first.
    if (!pending_dedents.empty())
    {
        Token t = pending_dedents.front();
        pending_dedents.pop_front();
        return t;
    }

    // We expect to be called when peek() == '\n'. Consume that newline(s).
    // Consume a single newline (the one that triggered calling this function)
    if (peek() == '\n')
    {
        advance(); // increments line_ and resets column_
    }

    // Skip any additional consecutive newline characters (blank lines).
    // For blank lines we do not produce INDENT/DEDENT—return a single NEWLINE.
    while (peek() == '\n' && !is_at_end())
    {
        advance();
    }

    // Now count indentation (spaces/tabs) at the start of the next non-blank line.
    const int TAB_WIDTH = 4;
    size_t indent = 0;
    bool has_tab = false, has_space = false;
    while (!is_at_end() && (peek() == ' ' || peek() == '\t'))
    {
        char c = advance();
        if (c == '\t')
            has_tab = true;
        if (c == ' ')
            has_space = true;
        if (has_tab && has_space)
        {
            throw std::runtime_error("Mixed tabs and spaces at line " + std::to_string(line_) + ", column " + std::to_string(column_ - 1));
        }
        if (c == ' ')
            indent += 1;
        else if (c == '\t')
            indent += TAB_WIDTH;
    }

    // If we reached EOF or the next line is blank (immediately another newline),
    // just return a NEWLINE token — nothing to change in indentation levels.
    if (is_at_end() || peek() == '\n')
    {
        return make_token(TokenType::NEWLINE, "");
    }

    // Compare with current indentation level
    if (indent_levels_.empty())
    {
        throw std::runtime_error("Internal lexer error: empty indent stack");
    }
    size_t current_indent = indent_levels_.top();

    if (indent > current_indent)
    {
        // Increased indentation -> push and emit INDENT
        indent_levels_.push(indent);
        return make_token(TokenType::INDENT, "");
    }
    else if (indent < current_indent)
    {
        // Decreased indentation -> pop until we reach the matching indentation level.
        // Emit one DEDENT now and enqueue remaining DEDENTs (if any).
        while (!indent_levels_.empty() && indent < indent_levels_.top())
        {
            indent_levels_.pop();
            pending_dedents.push_back(make_token(TokenType::DEDENT, ""));
        }
        if (indent_levels_.empty() || indent != indent_levels_.top())
        {
            throw std::runtime_error("Invalid dedent at line " + std::to_string(line_) + ", column " + std::to_string(column_));
        }
        // Pop front and return the first DEDENT (the rest remain in pending_dedents).
        Token t = pending_dedents.front();
        pending_dedents.pop_front();
        return t;
    }

    // Same indentation level -> just return NEWLINE token
    return make_token(TokenType::NEWLINE, "");
}

Token Lexer::make_token(TokenType type, const std::variant<int, std::string>& value) const
{
    return Token { type, value, line_, column_ };
}

} // namespace minic
