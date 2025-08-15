#include "Lexer.hpp"
#include <gtest/gtest.h>

namespace minic
{

// PublicLexer exposes protected members of Lexer for testing
class PublicLexer : public Lexer
{
public:
    using Lexer::Lexer; // Expose constructor

    // Expose protected methods
    using Lexer::advance;
    using Lexer::handle_indentation;
    using Lexer::is_at_end;
    using Lexer::Lex; // Expose Lex method
    using Lexer::make_token;
    using Lexer::next_token;
    using Lexer::peek;
    using Lexer::scan_identifier;
    using Lexer::scan_number;
    using Lexer::scan_string;
    using Lexer::skip_comment;
    using Lexer::skip_whitespace;

    // Expose member variables
    using Lexer::column_;
    using Lexer::indent_levels_;
    using Lexer::line_;
    using Lexer::pos_;
    using Lexer::source_;
};

} // namespace minic

class LexerTest : public ::testing::Test
{
protected:
    minic::PublicLexer lexer;
    LexerTest()
        : lexer("int main() { return 0; }")
    {
    }
};

// Test peek() returns first character and initial positions
TEST_F(LexerTest, Peek)
{
    ASSERT_EQ(lexer.peek(), 'i');
    ASSERT_EQ(lexer.column_, 1);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, 0);
    ASSERT_EQ(lexer.indent_levels_.size(), 1);
}

// Test advance() moves position and returns character
TEST_F(LexerTest, Advance)
{
    char c = lexer.advance();
    ASSERT_EQ(c, 'i');
    ASSERT_EQ(lexer.column_, 2);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, 1);
}

// Test advance() at end of input
TEST_F(LexerTest, AdvanceAtEnd)
{
    lexer.pos_ = lexer.source_.size();
    char c = lexer.advance();
    ASSERT_EQ(c, '\0');
    ASSERT_EQ(lexer.column_, 1);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, lexer.source_.size());
}

// Test advance() on newline
TEST_F(LexerTest, AdvanceNewLine)
{
    lexer.source_ = "int main()\n{\nreturn 0;\n}";
    lexer.pos_ = 12;
    char c = lexer.advance();
    ASSERT_EQ(c, '\n');
    ASSERT_EQ(lexer.column_, 1);
    ASSERT_EQ(lexer.line_, 2);
    ASSERT_EQ(lexer.pos_, 13);
}

// Test is_at_end() returns correct status
TEST_F(LexerTest, IsAtEnd)
{
    ASSERT_FALSE(lexer.is_at_end());
    lexer.pos_ = lexer.source_.size();
    ASSERT_TRUE(lexer.is_at_end());
}

// Test skip_whitespace() skips spaces
TEST_F(LexerTest, SkipWhitespace)
{
    lexer.source_ = "   int main() \n  { return 0; }";
    lexer.pos_ = 0;
    lexer.skip_whitespace();
    ASSERT_EQ(lexer.peek(), 'i');
    ASSERT_EQ(lexer.column_, 4);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, 3);
}

// Test skip_comment() does not skip if not at comment
TEST_F(LexerTest, SkipCommentSingleLine)
{
    lexer.source_ = "int main() // This is a comment\n{ return 0; }";
    lexer.pos_ = 0;
    lexer.skip_comment();
    ASSERT_EQ(lexer.peek(), 'i');
    ASSERT_EQ(lexer.column_, 1);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, 0);
}

// Test skip_comment() skips multi-line comment
TEST_F(LexerTest, SkipCommentMultiLine)
{
    lexer.source_ = "int main() /* This is a \n multi-line comment */ { return 0; }";
    lexer.pos_ = 0;
    lexer.line_ = 1;
    lexer.column_ = 1;
    while (lexer.peek() != '/')
        lexer.advance();
    lexer.skip_comment();
    lexer.skip_whitespace();
    ASSERT_EQ(lexer.peek(), '{');
    ASSERT_EQ(lexer.column_, 24);
    ASSERT_EQ(lexer.line_, 2);
    ASSERT_EQ(lexer.pos_, 48);
}

// Test skip_comment() at end of input
TEST_F(LexerTest, SkipCommentAtEnd)
{
    lexer.source_ = "int main() { return 0; } // End comment";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;
    while (lexer.peek() != '/')
        lexer.advance();
    lexer.skip_comment();
    ASSERT_EQ(lexer.peek(), '\0');
    ASSERT_EQ(lexer.column_, 40);
    ASSERT_EQ(lexer.line_, 1);
    ASSERT_EQ(lexer.pos_, lexer.source_.size());
}

// Test scan_number() parses integer literal
TEST_F(LexerTest, ScanDigit)
{
    lexer.source_ = "abcde 12345 abc";
    lexer.pos_ = 6;
    lexer.column_ = 7;
    minic::Token token = lexer.scan_number();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(token.value), 12345);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 7);
}

// Test scan_string() parses string literal
TEST_F(LexerTest, ScanString)
{
    lexer.source_ = "abcde \"Hello, World!\" abc";
    lexer.pos_ = 7;
    lexer.column_ = 8;
    minic::Token token = lexer.scan_string();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_STRING);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 9);
}

TEST_F(LexerTest, UnclosedStringLiteral)
{
    lexer.source_ = "abcde \"Unclosed string literal";
    lexer.pos_ = 6;
    lexer.column_ = 6;
    EXPECT_THROW(lexer.scan_string(), std::runtime_error);
}

TEST_F(LexerTest, ScanIdentifier)
{
    lexer.source_ = "int main()  return 0";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    minic::Token token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);

    lexer.pos_ = 4; // Move to 'main'
    lexer.column_ = 5;
    token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(token.value), "main");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 5);

    lexer.pos_ = 12; // Move to 'return'
    lexer.column_ = 13;
    token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 13);
}

TEST_F(LexerTest, HandleIndentation)
{
    lexer.source_ = "if (true) {\n    return 0;\n}";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    while (lexer.peek() != '\n' && !lexer.is_at_end())
        lexer.advance(); // Move to the end of the first line
    lexer.advance(); // Skip newline

    minic::Token indent_token = lexer.handle_indentation();
    ASSERT_EQ(indent_token.type, minic::TokenType::INDENT);
    ASSERT_EQ(lexer.indent_levels_.top(), 4); // Assuming 4 spaces for indentation
}

TEST_F(LexerTest, HandleDedent)
{
    // Simulate a block with increased indentation, then dedent
    lexer.source_ = "if (true) {\n    return 0;\n}\n";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    // Move to the end of the first line
    while (lexer.peek() != '\n' && !lexer.is_at_end())
        lexer.advance();
    lexer.advance(); // Skip newline

    // Handle indent after first newline
    minic::Token indent_token = lexer.handle_indentation();
    ASSERT_EQ(indent_token.type, minic::TokenType::INDENT);

    // Move to the end of the second line
    while (lexer.peek() != '\n' && !lexer.is_at_end())
        lexer.advance();
    lexer.advance(); // Skip newline

    // Handle dedent after second newline
    minic::Token dedent_token = lexer.handle_indentation();
    ASSERT_EQ(dedent_token.type, minic::TokenType::DEDENT);
}

TEST_F(LexerTest, NextToken)
{
    lexer.source_ = "int main() return 0";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    minic::Token token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(token.value), "main");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 5);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::LPAREN);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 9);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::RPAREN);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 10);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 12);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(token.value), 0);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 19);

    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, Lex)
{
    lexer.source_ = "int main() return 0";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 7); // int, main, (, ), return, 0, END_OF_FILE

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(tokens[0].line, 1);
    ASSERT_EQ(tokens[0].column, 1);

    ASSERT_EQ(tokens[1].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[1].value), "main");
    ASSERT_EQ(tokens[1].line, 1);
    ASSERT_EQ(tokens[1].column, 5);

    ASSERT_EQ(tokens[2].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[2].line, 1);
    ASSERT_EQ(tokens[2].column, 9);

    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[3].line, 1);
    ASSERT_EQ(tokens[3].column, 10);

    ASSERT_EQ(tokens[4].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[4].line, 1);
    ASSERT_EQ(tokens[4].column, 12);

    ASSERT_EQ(tokens[5].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[5].value), 0);
    ASSERT_EQ(tokens[5].line, 1);
    ASSERT_EQ(tokens[5].column, 19);

    ASSERT_EQ(tokens[6].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, LexWithIndentation)
{
    // Test with a simple if statement with indentation
    lexer.source_ = "if (true)\n    return 0\n    if"; // Use 4 spaces for indentation (spaces only, no tabs)
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 10); // if, (, true, ), INDENT, return, 0, DEDENT, if, END_OF_FILE

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[1].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[2].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[2].value), "true");
    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::INDENT);

    ASSERT_EQ(tokens[5].type, minic::TokenType::KEYWORD_RETURN);

    ASSERT_EQ(tokens[6].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[6].value), 0);

    ASSERT_EQ(tokens[7].type, minic::TokenType::NEWLINE);

    ASSERT_EQ(tokens[8].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[9].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, LexWithMixedIndentation)
{
    lexer.source_ = "if (true) \n\t return 0\n";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    EXPECT_THROW(lexer.Lex(), std::runtime_error); // Should throw due to mixed tabs and spaces
}