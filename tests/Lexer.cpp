#include "miniC/Lexer.hpp"
#include <gtest/gtest.h>

namespace minic
{

class PublicLexer : public Lexer
{

public:
    using Lexer::Lexer; // Expose the constructor for testing purposes

    // Expose protected methods for testing
    using Lexer::advance;
    using Lexer::is_at_end;
    using Lexer::peek;
    using Lexer::skip_whitespace;
    using Lexer::skip_comment;
    /* not yet implemented
    using Lexer::next_token;
    using Lexer::scan_identifier;
    using Lexer::scan_number;
    using Lexer::scan_string;
    using Lexer::handle_indentation;
    using Lexer::make_token;
    using Lexer::check_indent_consistency;
    */

    // Expose member variables for testing
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

// peek

TEST_F(LexerTest, Peek)
{
    ASSERT_EQ(lexer.peek(), 'i'); // 'i' from "int"
    ASSERT_EQ(lexer.column_, 1); // Initial column should be 1
    ASSERT_EQ(lexer.line_, 1); // Initial line should be 1
    ASSERT_EQ(lexer.pos_, 0); // Initial position should be 0
    ASSERT_EQ(lexer.indent_levels_.size(), 1); // Initial indent levels should be 1 (root level)
}

// advance

TEST_F(LexerTest, Advance)
{
    char c = lexer.advance();
    ASSERT_EQ(c, 'i'); // Should return 'i' from "int"
    ASSERT_EQ(lexer.column_, 2); // Column should now be 2
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    ASSERT_EQ(lexer.pos_, 1); // Position should now be 1
}

TEST_F(LexerTest, AdvanceAtEnd)
{
    lexer.pos_ = lexer.source_.size(); // Move to the end
    char c = lexer.advance();
    ASSERT_EQ(c, '\0'); // Should return null character at end of input
    ASSERT_EQ(lexer.column_, 1); // Column should reset to 1
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    ASSERT_EQ(lexer.pos_, lexer.source_.size()); // Position should remain at end
}

TEST_F(LexerTest, AdvanceNewLine)
{
    lexer.source_ = "int main()\n{\nreturn 0;\n}";
    lexer.pos_ = 12; // Position at new line character
    char c = lexer.advance();
    ASSERT_EQ(c, '\n'); // Should return new line character
    ASSERT_EQ(lexer.column_, 1); // Column should reset to 1
    ASSERT_EQ(lexer.line_, 2); // Line should increment to 2
    ASSERT_EQ(lexer.pos_, 13); // Position should now be 12
}

TEST_F(LexerTest, IsAtEnd)
{
    ASSERT_FALSE(lexer.is_at_end()); // Should return false as we are not at the end
    lexer.pos_ = lexer.source_.size(); // Move to the end
    ASSERT_TRUE(lexer.is_at_end()); // Should return true at the end of input
}

TEST_F(LexerTest, SkipWhitespace)
{
    lexer.source_ = "   int main() \n  { return 0; }";
    lexer.pos_ = 0; // Reset position to start
    lexer.skip_whitespace(); // Should skip leading spaces
    ASSERT_EQ(lexer.peek(), 'i'); // Next character should be 'i'
    ASSERT_EQ(lexer.column_, 4); // Column should be 4 after skipping spaces
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    ASSERT_EQ(lexer.pos_, 3); // Position should be at 'i'
}

TEST_F(LexerTest, SkipCommentSingleLine)
{
    lexer.source_ = "int main() // This is a comment\n{ return 0; }";
    lexer.pos_ = 0; // Reset position to start
    lexer.skip_comment(); // Should skip the single-line comment
    ASSERT_EQ(lexer.peek(), 'i'); // Next character should be 'i'
    ASSERT_EQ(lexer.column_, 1); // Column should be 1
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    ASSERT_EQ(lexer.pos_, 0); // Position should still be at start
}

TEST_F(LexerTest, SkipCommentMultiLine)
{
    lexer.source_ = "int main() /* This is a \n multi-line comment */ { return 0; }";
    lexer.pos_ = 0; // Reset position to start
    lexer.line_ = 1; // Reset line to 1
    lexer.column_ = 1; // Reset column to 1
    while (lexer.peek() != '/') // Advance to the start of the comment
        lexer.advance();
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    lexer.skip_comment(); // Should skip the multi-line comment
    ASSERT_EQ(lexer.line_, 2); // Line should still be 1
    lexer.skip_whitespace(); // Skip any whitespace after the comment
    ASSERT_EQ(lexer.line_, 2); // Line should still be 1
    ASSERT_EQ(lexer.peek(), '{'); // Next character should be '{'
    ASSERT_EQ(lexer.column_, 24); // Column should be at the start of '{
    ASSERT_EQ(lexer.line_, 2); // Line should be 2 after the comment
    ASSERT_EQ(lexer.pos_, 48); // Position should be at the start of '{
}

TEST_F(LexerTest, SkipCommentAtEnd)
{
    lexer.source_ = "int main() { return 0; } // End comment";
    lexer.pos_ = 0; // Reset position to start
    lexer.column_ = 1; // Reset column to 1
    lexer.line_ = 1; // Reset line to 1
    while (lexer.peek() != '/') // Advance to the start of the comment
        lexer.advance();
    lexer.skip_comment(); // Should skip the comment at the end
    ASSERT_EQ(lexer.peek(), '\0'); // Next character should be null at end of input
    ASSERT_EQ(lexer.column_, 40); // Column should be 1
    ASSERT_EQ(lexer.line_, 1); // Line should still be 1
    ASSERT_EQ(lexer.pos_, lexer.source_.size()); // Position should be at the end
}
