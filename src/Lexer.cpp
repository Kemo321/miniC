#include "miniC/Lexer.hpp"

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
    // TODO: Implement skip_whitespace
}

void Lexer::skip_comment()
{
    // TODO: Implement skip_comment
}

Token Lexer::scan_identifier()
{
    // TODO: Implement scan_identifier
    return Token {};
}

Token Lexer::scan_number()
{
    // TODO: Implement scan_number
    return Token {};
}

Token Lexer::scan_string()
{
    // TODO: Implement scan_string
    return Token {};
}

Token Lexer::handle_indentation()
{
    // TODO: Implement handle_indentation
    return Token {};
}

Token Lexer::make_token(TokenType type, const std::variant<int, std::string>& value) const
{
    // TODO: Implement make_token
    return Token {};
}

void Lexer::check_indent_consistency(char c)
{
    // TODO: Implement check_indent_consistency
}

} // namespace minic