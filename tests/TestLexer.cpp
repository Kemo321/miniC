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
    lexer.source_ = "int main()\n{return 0;}";
    lexer.pos_ = 10;
    char c = lexer.advance();
    ASSERT_EQ(c, '\n');
    ASSERT_EQ(lexer.column_, 1);
    ASSERT_EQ(lexer.line_, 2);
    ASSERT_EQ(lexer.pos_, 11);
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
    ASSERT_EQ(token.column, 8);
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

TEST_F(LexerTest, LexWithBrackets)
{
    lexer.source_ = "if (true) { return 0; } if";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 11); // if, (, true, ), {, return, 0, ;, }, if, EOF

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[1].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[2].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[2].value), "true");
    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[5].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[6].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[6].value), 0);
    ASSERT_EQ(tokens[7].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[8].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[9].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[10].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, ScanIntegerLiteral)
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

TEST_F(LexerTest, ScanIdentifierKeywordIf)
{
    lexer.source_ = "if";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    minic::Token token = lexer.scan_identifier();
    ASSERT_EQ(token.type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);
}

TEST_F(LexerTest, NextTokenHandlesBrackets)
{
    lexer.source_ = "( ) { } ;";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::TokenType> expected = {
        minic::TokenType::LPAREN, minic::TokenType::RPAREN, minic::TokenType::LBRACE,
        minic::TokenType::RBRACE, minic::TokenType::SEMICOLON, minic::TokenType::END_OF_FILE
    };

    for (size_t i = 0; i < expected.size(); ++i)
    {
        minic::Token token = lexer.next_token();
        ASSERT_EQ(token.type, expected[i]);
    }
}

TEST_F(LexerTest, LexHandlesSingleLine)
{
    lexer.source_ = "string name = \"John\";";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 6); // string, name, =, "John", ;, EOF

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_STR);
    ASSERT_EQ(tokens[1].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(tokens[3].type, minic::TokenType::LITERAL_STRING);
    ASSERT_EQ(std::get<std::string>(tokens[3].value), "John");
    ASSERT_EQ(tokens[2].type, minic::TokenType::OP_ASSIGN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[5].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, FullCode)
{
    lexer.source_ = "int main() { if (true) { return 0; } }";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 16); // int, main, (, ), {, if, (, true, ), {, return, 0, ;, }, }, EOF

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(tokens[1].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[1].value), "main");
    ASSERT_EQ(tokens[2].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[5].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[6].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[7].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[7].value), "true");
    ASSERT_EQ(tokens[8].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[9].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[10].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[11].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[11].value), 0);
    ASSERT_EQ(tokens[12].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[13].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[14].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[15].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, LexComplexProgram)
{
    lexer.source_ = "int main() { x = 5 + 3; if (x > 0) { print(\"x is positive\"); } return x; }";
    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 29);

    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(tokens[1].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[1].value), "main");
    ASSERT_EQ(tokens[2].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[5].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[5].value), "x");
    ASSERT_EQ(tokens[6].type, minic::TokenType::OP_ASSIGN);
    ASSERT_EQ(tokens[7].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[7].value), 5);
    ASSERT_EQ(tokens[8].type, minic::TokenType::OP_PLUS);
    ASSERT_EQ(tokens[9].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[9].value), 3);
    ASSERT_EQ(tokens[10].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[11].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[12].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[13].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[13].value), "x");
    ASSERT_EQ(tokens[14].type, minic::TokenType::OP_GREATER);
    ASSERT_EQ(tokens[15].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[15].value), 0);
    ASSERT_EQ(tokens[16].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[17].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[18].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[18].value), "print");
    ASSERT_EQ(tokens[19].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[20].type, minic::TokenType::LITERAL_STRING);
    ASSERT_EQ(std::get<std::string>(tokens[20].value), "x is positive");
    ASSERT_EQ(tokens[21].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[22].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[23].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[24].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[25].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[25].value), "x");
    ASSERT_EQ(tokens[26].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[27].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[28].type, minic::TokenType::END_OF_FILE);
}

TEST_F(LexerTest, NextTokenOperators)
{
    lexer.source_ = "= == != < <= > >= : ,";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    minic::Token token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_ASSIGN);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_EQUAL);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_NOT_EQUAL);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_LESS);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_LESS_EQ);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_GREATER);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::OP_GREATER_EQ);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::COLON);
    token = lexer.next_token();
    ASSERT_EQ(token.type, minic::TokenType::COMMA);
}

TEST_F(LexerTest, ScanStringWithEscapes)
{
    lexer.source_ = "\"Hello\\n\\t\\\"World\\\"\"";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;
    minic::Token token = lexer.scan_string();
    ASSERT_EQ(token.type, minic::TokenType::LITERAL_STRING);
    ASSERT_EQ(std::get<std::string>(token.value), "Hello\n\t\"World\"");
    ASSERT_EQ(token.line, 1);
    ASSERT_EQ(token.column, 1);
}

TEST_F(LexerTest, AnotherComplexProgram)
{
    lexer.source_ = "int main() {\n"
                    "    int x = 5;\n"
                    "    x = x + 1;\n"
                    "    if (x > 0) {\n"
                    "        return x;\n"
                    "    } else {\n"
                    "        return 0;\n"
                    "    }\n"
                    "}";
    lexer.pos_ = 0;
    lexer.column_ = 1;
    lexer.line_ = 1;

    std::vector<minic::Token> tokens = lexer.Lex();
    ASSERT_EQ(tokens.size(), 43);
    ASSERT_EQ(tokens[0].type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(tokens[1].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[1].value), "main");
    ASSERT_EQ(tokens[2].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[3].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[4].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[5].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[6].type, minic::TokenType::KEYWORD_INT);
    ASSERT_EQ(tokens[7].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[7].value), "x");
    ASSERT_EQ(tokens[8].type, minic::TokenType::OP_ASSIGN);
    ASSERT_EQ(tokens[9].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[9].value), 5);
    ASSERT_EQ(tokens[10].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[11].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[12].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[12].value), "x");
    ASSERT_EQ(tokens[13].type, minic::TokenType::OP_ASSIGN);
    ASSERT_EQ(tokens[14].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[14].value), "x");
    ASSERT_EQ(tokens[15].type, minic::TokenType::OP_PLUS);
    ASSERT_EQ(tokens[16].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[16].value), 1);
    ASSERT_EQ(tokens[17].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[18].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[19].type, minic::TokenType::KEYWORD_IF);
    ASSERT_EQ(tokens[20].type, minic::TokenType::LPAREN);
    ASSERT_EQ(tokens[21].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[21].value), "x");
    ASSERT_EQ(tokens[22].type, minic::TokenType::OP_GREATER);
    ASSERT_EQ(tokens[23].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[23].value), 0);
    ASSERT_EQ(tokens[24].type, minic::TokenType::RPAREN);
    ASSERT_EQ(tokens[25].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[26].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[27].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[28].type, minic::TokenType::IDENTIFIER);
    ASSERT_EQ(std::get<std::string>(tokens[28].value), "x");
    ASSERT_EQ(tokens[29].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[30].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[31].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[32].type, minic::TokenType::KEYWORD_ELSE);
    ASSERT_EQ(tokens[33].type, minic::TokenType::LBRACE);
    ASSERT_EQ(tokens[34].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[35].type, minic::TokenType::KEYWORD_RETURN);
    ASSERT_EQ(tokens[36].type, minic::TokenType::LITERAL_INT);
    ASSERT_EQ(std::get<int>(tokens[36].value), 0);
    ASSERT_EQ(tokens[37].type, minic::TokenType::SEMICOLON);
    ASSERT_EQ(tokens[38].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[39].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[40].type, minic::TokenType::NEWLINE);
    ASSERT_EQ(tokens[41].type, minic::TokenType::RBRACE);
    ASSERT_EQ(tokens[42].type, minic::TokenType::END_OF_FILE);
}
