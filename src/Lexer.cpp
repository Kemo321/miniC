#include "miniC/Lexer.hpp"
#include <iostream>

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
    // TODO: Implement Lex method
    return {};
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
    // TODO: Implement next_token
    return Token {};
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
    if (peek() == '/' && source_[pos_ + 1] == '/')
    {
        // Single-line comment
        while (peek() != '\n' && !is_at_end())
        {
            advance();
        }
    }
    else if (peek() == '/' && source_[pos_ + 1] == '*')
    {
        // Multi-line comment
        advance(); // Skip '/'
        advance(); // Skip '*'
        while (!(peek() == '*' && source_[pos_ + 1] == '/') && !is_at_end())
        {
            advance();
        }
        advance(); // Skip '*'
        advance(); // Skip '/'
    }
}

Token Lexer::scan_identifier()
{
    std::string identifier;
    size_t start_col = column_ - 1;
    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_'))
    {
        identifier += advance();
    }
    
    // Check if the identifier is a keyword
    TokenType type = TokenType::IDENTIFIER; // Default to IDENTIFIER
    if (identifier == "int") type = TokenType::KEYWORD_INT;
    else if (identifier == "void") type = TokenType::KEYWORD_VOID;
    else if (identifier == "if") type = TokenType::KEYWORD_IF;
    else if (identifier == "else") type = TokenType::KEYWORD_ELSE;
    else if (identifier == "while") type = TokenType::KEYWORD_WHILE;
    else if (identifier == "return") type = TokenType::KEYWORD_RETURN;

    return make_token(type, identifier);
}

Token Lexer::scan_number()
{
    std::string num;
    size_t start_col = column_ - 1;
    num += source_[pos_ - 1]; // Include first digit
    while (!is_at_end() && std::isdigit(peek()))
    {
        num += advance();
    }
    return make_token(TokenType::LITERAL_INT, std::stoi(num));
}

Token Lexer::scan_string()
{
    std::string str;
    size_t start_col = column_ - 1;
    advance(); // Skip opening quote
    while (!is_at_end() && peek() != '"')
    {
        str += advance();
    }
    if (!is_at_end())
    {
        advance(); // Skip closing quote
        return make_token(TokenType::LITERAL_STRING, str);
    }
    // Handle error: unclosed string literal
    throw std::runtime_error("Unclosed string literal at line " + std::to_string(line_) + ", column " + std::to_string(start_col));
}

Token Lexer::handle_indentation()
{
    size_t indent = 0;
    bool is_tab = false;
    while (!is_at_end() && (peek() == ' ' || peek() == '\t')) {
        char c = advance();
        check_indent_consistency(c);
        if (c == ' ') indent++;
        else if (c == '\t') indent += 4; // Assume 1 tab = 4 spaces
        is_tab = (c == '\t');
    }

    // Skip empty lines
    if (peek() == '\n' || is_at_end()) {
        return make_token(TokenType::NEWLINE, "");
    }

    size_t current_indent = indent_levels_.top();
    if (indent > current_indent) {
        indent_levels_.push(indent);
        return make_token(TokenType::INDENT, "");
    } else if (indent < current_indent) {
        indent_levels_.pop();
        if (indent != indent_levels_.top()) {
            throw std::runtime_error("Invalid dedent at line " + std::to_string(line_) + ", column " + std::to_string(column_));
        }
        return make_token(TokenType::DEDENT, "");
    }
    return make_token(TokenType::NEWLINE, ""); // Same indent level
}

Token Lexer::make_token(TokenType type, const std::variant<int, std::string>& value) const
{
    Token token;
    token.type = type;
    token.value = value;
    token.line = line_;
    token.column = column_;
    return token;
}

void Lexer::check_indent_consistency(char c)
{
    static bool has_tab = false, has_space = false;
    if (c == '\t') has_tab = true;
    if (c == ' ') has_space = true;
    if (has_tab && has_space) {
        throw std::runtime_error("Mixed tabs and spaces at line " + std::to_string(line_) + ", column " + std::to_string(column_ - 1));
    }
}

} // namespace minic