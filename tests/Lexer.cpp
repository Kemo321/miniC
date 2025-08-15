#include "miniC/Lexer.hpp"
#include "miniC/Token.hpp"
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
    using Lexer::is_at_end;
    using Lexer::peek;
    using Lexer::scan_number;
    using Lexer::skip_comment;
    using Lexer::skip_whitespace;
    using Lexer::scan_string;
    using Lexer::scan_identifier;

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
    lexer.column_ = 6;
    minic::Token token = lexer.scan_number();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(token.value), 12345);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 11);
}

// Test scan_string() parses string literal
TEST_F(LexerTest, ScanString)
{
    lexer.source_ = "abcde \"Hello, World!\" abc";
    lexer.pos_ = 6;
    lexer.column_ = 6;
    minic::Token token = lexer.scan_string();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_STRING);
    ASSERT_EQ(std::get<std::string>(token.value), "Hello, World!");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 21);
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
    lexer.source_ = "int main() { return 0; }";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    minic::Token token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(std::get<std::string>(token.value), "int");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 4);

    lexer.pos_ = 4; // Move to 'main'
    lexer.column_ = 5;
    token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(token.value), "main");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 9);

    lexer.pos_ = 13; // Move to 'return'
    lexer.column_ = 14;
    token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(std::get<std::string>(token.value), "return");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 20);

    lexer.pos_ = 21; // Move to '0'
    lexer.column_ = 22;
    token = lexer.scan_number();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(token.value), 0);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 22);
}