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
    /* not yet implemented
    using Lexer::next_token;
    using Lexer::skip_whitespace;
    using Lexer::skip_comment;
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