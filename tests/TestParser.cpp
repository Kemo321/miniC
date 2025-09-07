#include "minic/Parser.hpp"
#include <gtest/gtest.h>

namespace minic
{

// PublicParser to expose private methods for testing
class PublicParser : public Parser
{
public:
    using Parser::advance;
    using Parser::check;
    using Parser::consume;
    using Parser::is_at_end;
    using Parser::parse_assign_statement;
    using Parser::parse_block;
    using Parser::parse_comparison;
    using Parser::parse_expression;
    using Parser::parse_factor;
    using Parser::parse_function;
    using Parser::parse_if_statement;
    using Parser::parse_parameters;
    using Parser::parse_primary;
    using Parser::parse_return_statement;
    using Parser::parse_statement;
    using Parser::parse_term;
    using Parser::parse_while_statement;
    using Parser::Parser;
    using Parser::peek;
    using Parser::previous;
    using Parser::synchronize;

    // Helper to set current position for testing
    void set_current(size_t pos) { current_ = pos; }
};

} // namespace minic

class ParserTest : public ::testing::Test
{
protected:
    std::vector<minic::Token> tokens_;
    minic::PublicParser parser_ { tokens_ };

    // Helper to create token
    minic::Token MakeToken(minic::TokenType type, std::variant<int, std::string> value = {},
        size_t line = 1, size_t column = 1)
    {
        return { type, value, line, column };
    }

    // Reset tokens and parser for each test
    void SetUp() override
    {
        tokens_.clear();
    }
};

// Test is_at_end
TEST_F(ParserTest, IsAtEnd)
{
    EXPECT_TRUE(parser_.is_at_end()); // Empty tokens
    tokens_.push_back(MakeToken(minic::TokenType::END_OF_FILE));
    EXPECT_TRUE(parser_.is_at_end());
    tokens_.clear();
    tokens_.push_back(MakeToken(minic::TokenType::IDENTIFIER));
    EXPECT_FALSE(parser_.is_at_end());
}

// Test advance and peek
TEST_F(ParserTest, AdvanceAndPeek)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT, {}, 1, 1),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("main"), 1, 5) };
    EXPECT_EQ(parser_.peek().type, minic::TokenType::KEYWORD_INT);
    parser_.advance();
    EXPECT_EQ(parser_.peek().type, minic::TokenType::IDENTIFIER);
    EXPECT_EQ(parser_.previous().type, minic::TokenType::KEYWORD_INT);
    parser_.advance();
    EXPECT_TRUE(parser_.is_at_end());
}

// Test check
TEST_F(ParserTest, Check)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT) };
    EXPECT_TRUE(parser_.check(minic::TokenType::KEYWORD_INT));
    EXPECT_FALSE(parser_.check(minic::TokenType::IDENTIFIER));
}

// Test consume success and failure
TEST_F(ParserTest, Consume)
{
    tokens_ = { MakeToken(minic::TokenType::LPAREN, {}, 1, 1) };
    EXPECT_EQ(parser_.consume(minic::TokenType::LPAREN, "Error").type, minic::TokenType::LPAREN);
    EXPECT_THROW(parser_.consume(minic::TokenType::RPAREN, "Expected )"), std::runtime_error);
}

// Test synchronize (advances to SEMICOLON or end)
TEST_F(ParserTest, Synchronize)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER), MakeToken(minic::TokenType::OP_PLUS),
        MakeToken(minic::TokenType::SEMICOLON), MakeToken(minic::TokenType::KEYWORD_RETURN) };
    parser_.set_current(0);
    parser_.synchronize();
    EXPECT_EQ(parser_.peek().type, minic::TokenType::KEYWORD_RETURN); // Advanced past ;
}

// Test parse_primary (int, string, id, error)
TEST_F(ParserTest, ParsePrimaryInt)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 42) };
    auto expr = parser_.parse_primary();
    auto lit = dynamic_cast<minic::IntLiteral*>(expr.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 42);
}

TEST_F(ParserTest, ParsePrimaryString)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_STRING, std::string("hello")) };
    auto expr = parser_.parse_primary();
    auto lit = dynamic_cast<minic::StringLiteral*>(expr.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, "hello");
}

TEST_F(ParserTest, ParsePrimaryIdentifier)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("x")) };
    auto expr = parser_.parse_primary();
    auto id = dynamic_cast<minic::Identifier*>(expr.get());
    ASSERT_NE(id, nullptr);
    EXPECT_EQ(id->name, "x");
}

TEST_F(ParserTest, ParsePrimaryError)
{
    tokens_ = { MakeToken(minic::TokenType::OP_PLUS) };
    EXPECT_THROW(parser_.parse_primary(), std::runtime_error);
}

// Test parse_factor (primary, with/without * /)
TEST_F(ParserTest, ParseFactorSimple)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 5) };
    auto expr = parser_.parse_factor();
    auto lit = dynamic_cast<minic::IntLiteral*>(expr.get());
    ASSERT_NE(lit, nullptr);
    EXPECT_EQ(lit->value, 5);
}

TEST_F(ParserTest, ParseFactorMultiply)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 2),
        MakeToken(minic::TokenType::OP_MULTIPLY),
        MakeToken(minic::TokenType::LITERAL_INT, 3) };
    auto expr = parser_.parse_factor();
    auto bin = dynamic_cast<minic::BinaryExpr*>(expr.get());
    ASSERT_NE(bin, nullptr);
    EXPECT_EQ(bin->op, minic::TokenType::OP_MULTIPLY);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin->left.get())->value, 2);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin->right.get())->value, 3);
}

TEST_F(ParserTest, ParseFactorDivideMultiple)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 10),
        MakeToken(minic::TokenType::OP_DIVIDE),
        MakeToken(minic::TokenType::LITERAL_INT, 2),
        MakeToken(minic::TokenType::OP_MULTIPLY),
        MakeToken(minic::TokenType::LITERAL_INT, 3) };
    auto expr = parser_.parse_factor();
    auto bin_outer = dynamic_cast<minic::BinaryExpr*>(expr.get());
    ASSERT_NE(bin_outer, nullptr);
    EXPECT_EQ(bin_outer->op, minic::TokenType::OP_MULTIPLY);
    auto bin_inner = dynamic_cast<minic::BinaryExpr*>(bin_outer->left.get());
    ASSERT_NE(bin_inner, nullptr);
    EXPECT_EQ(bin_inner->op, minic::TokenType::OP_DIVIDE);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_inner->left.get())->value, 10);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_inner->right.get())->value, 2);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_outer->right.get())->value, 3);
}

// Test parse_term (factor + -)
TEST_F(ParserTest, ParseTermAddSubtract)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::OP_PLUS),
        MakeToken(minic::TokenType::LITERAL_INT, 2),
        MakeToken(minic::TokenType::OP_MINUS),
        MakeToken(minic::TokenType::LITERAL_INT, 3) };
    auto expr = parser_.parse_term();
    auto bin_outer = dynamic_cast<minic::BinaryExpr*>(expr.get());
    EXPECT_EQ(bin_outer->op, minic::TokenType::OP_MINUS);
    auto bin_inner = dynamic_cast<minic::BinaryExpr*>(bin_outer->left.get());
    EXPECT_EQ(bin_inner->op, minic::TokenType::OP_PLUS);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_inner->left.get())->value, 1);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_inner->right.get())->value, 2);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_outer->right.get())->value, 3);
}

// Test parse_comparison (term comparisons)
TEST_F(ParserTest, ParseComparisonMultiple)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_LESS_EQ),
        MakeToken(minic::TokenType::LITERAL_INT, 5),
        MakeToken(minic::TokenType::OP_NOT_EQUAL),
        MakeToken(minic::TokenType::LITERAL_INT, 0) };
    auto expr = parser_.parse_comparison();
    auto bin_outer = dynamic_cast<minic::BinaryExpr*>(expr.get());
    EXPECT_EQ(bin_outer->op, minic::TokenType::OP_NOT_EQUAL);
    auto bin_inner = dynamic_cast<minic::BinaryExpr*>(bin_outer->left.get());
    EXPECT_EQ(bin_inner->op, minic::TokenType::OP_LESS_EQ);
    EXPECT_EQ(dynamic_cast<minic::Identifier*>(bin_inner->left.get())->name, "x");
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_inner->right.get())->value, 5);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(bin_outer->right.get())->value, 0);
}

// Test parse_expression (aliases comparison)
TEST_F(ParserTest, ParseExpression)
{
    tokens_ = { MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::OP_PLUS),
        MakeToken(minic::TokenType::LITERAL_INT, 2) };
    auto expr = parser_.parse_expression();
    auto bin = dynamic_cast<minic::BinaryExpr*>(expr.get());
    EXPECT_EQ(bin->op, minic::TokenType::OP_PLUS);
}

// Test parse_return_statement (with/without value, error missing ;)
TEST_F(ParserTest, ParseReturnWithValue)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 0),
        MakeToken(minic::TokenType::SEMICOLON) };
    auto stmt = parser_.parse_return_statement();
    auto ret = dynamic_cast<minic::ReturnStmt*>(stmt.get());
    ASSERT_NE(ret, nullptr);
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(ret->value.get())->value, 0);
}

TEST_F(ParserTest, ParseReturnNoValue)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::SEMICOLON) };
    auto stmt = parser_.parse_return_statement();
    auto ret = dynamic_cast<minic::ReturnStmt*>(stmt.get());
    ASSERT_NE(ret, nullptr);
    EXPECT_EQ(ret->value, nullptr);
}

TEST_F(ParserTest, ParseReturnMissingSemicolon)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 0) };
    EXPECT_THROW(parser_.parse_return_statement(), std::runtime_error);
}

// Test parse_assign_statement
TEST_F(ParserTest, ParseAssign)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_ASSIGN),
        MakeToken(minic::TokenType::LITERAL_INT, 5),
        MakeToken(minic::TokenType::SEMICOLON) };
    auto stmt = parser_.parse_assign_statement();
    auto assign = dynamic_cast<minic::AssignStmt*>(stmt.get());
    ASSERT_NE(assign, nullptr);
    EXPECT_EQ(assign->name, "x");
    EXPECT_EQ(dynamic_cast<minic::IntLiteral*>(assign->value.get())->value, 5);
}

TEST_F(ParserTest, ParseAssignMissingEqual)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("x")) };
    EXPECT_THROW(parser_.parse_assign_statement(), std::runtime_error);
}

TEST_F(ParserTest, ParseAssignMissingSemicolon)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_ASSIGN),
        MakeToken(minic::TokenType::LITERAL_INT, 5) };
    EXPECT_THROW(parser_.parse_assign_statement(), std::runtime_error);
}

// Test parse_block (empty, multiple stmts, missing })
TEST_F(ParserTest, ParseBlockEmpty)
{
    tokens_ = { MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::RBRACE) };
    auto block = parser_.parse_block();
    EXPECT_TRUE(block.empty());
}

TEST_F(ParserTest, ParseBlockWithStmts)
{
    tokens_ = { MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_ASSIGN),
        MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE) };
    auto block = parser_.parse_block();
    EXPECT_EQ(block.size(), 2);
    EXPECT_NE(dynamic_cast<minic::ReturnStmt*>(block[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<minic::AssignStmt*>(block[1].get()), nullptr);
}

TEST_F(ParserTest, ParseBlockMissingLBrace)
{
    EXPECT_THROW(parser_.parse_block(), std::runtime_error);
}

TEST_F(ParserTest, ParseBlockMissingRBrace)
{
    tokens_ = { MakeToken(minic::TokenType::LBRACE) };
    EXPECT_THROW(parser_.parse_block(), std::runtime_error);
}

// Test parse_if_statement (with/without else, nested)
TEST_F(ParserTest, ParseIfNoElse)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_IF),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE) };
    auto stmt = parser_.parse_if_statement();
    auto if_stmt = dynamic_cast<minic::IfStmt*>(stmt.get());
    ASSERT_NE(if_stmt, nullptr);
    EXPECT_EQ(dynamic_cast<minic::Identifier*>(if_stmt->condition.get())->name, "x");
    EXPECT_EQ(if_stmt->then_branch.size(), 1);
    EXPECT_TRUE(if_stmt->else_branch.empty());
}

TEST_F(ParserTest, ParseIfWithElse)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_IF),
        MakeToken(minic::TokenType::LITERAL_INT, 0),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE),
        MakeToken(minic::TokenType::KEYWORD_ELSE),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 2),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE) };
    auto stmt = parser_.parse_if_statement();
    auto if_stmt = dynamic_cast<minic::IfStmt*>(stmt.get());
    ASSERT_NE(if_stmt, nullptr);
    EXPECT_EQ(if_stmt->then_branch.size(), 1);
    EXPECT_EQ(if_stmt->else_branch.size(), 1);
}

TEST_F(ParserTest, ParseIfMissingBlock)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_IF),
        MakeToken(minic::TokenType::LITERAL_INT, 1) };
    EXPECT_THROW(parser_.parse_if_statement(), std::runtime_error);
}

// Test parse_while_statement
TEST_F(ParserTest, ParseWhile)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_WHILE),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_LESS),
        MakeToken(minic::TokenType::LITERAL_INT, 10),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_ASSIGN),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("x")),
        MakeToken(minic::TokenType::OP_PLUS),
        MakeToken(minic::TokenType::LITERAL_INT, 1),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE) };
    auto stmt = parser_.parse_while_statement();
    auto while_stmt = dynamic_cast<minic::WhileStmt*>(stmt.get());
    ASSERT_NE(while_stmt, nullptr);
    auto cond = dynamic_cast<minic::BinaryExpr*>(while_stmt->condition.get());
    EXPECT_EQ(cond->op, minic::TokenType::OP_LESS);
    EXPECT_EQ(while_stmt->body.size(), 1);
}

// Test parse_parameters (none, one, multiple, invalid type)
TEST_F(ParserTest, ParseParametersEmpty)
{
    tokens_ = { MakeToken(minic::TokenType::RPAREN) };
    auto params = parser_.parse_parameters();
    EXPECT_TRUE(params.empty());
}

TEST_F(ParserTest, ParseParametersMultiple)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("a")),
        MakeToken(minic::TokenType::COMMA),
        MakeToken(minic::TokenType::KEYWORD_VOID),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("b")) };
    auto params = parser_.parse_parameters();
    EXPECT_EQ(params.size(), 2);
    EXPECT_EQ(params[0].type, minic::TokenType::KEYWORD_INT);
    EXPECT_EQ(params[0].name, "a");
    EXPECT_EQ(params[1].type, minic::TokenType::KEYWORD_VOID);
    EXPECT_EQ(params[1].name, "b");
}

TEST_F(ParserTest, ParseParametersInvalidType)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_IF),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("a")) };
    EXPECT_THROW(parser_.parse_parameters(), std::runtime_error);
}

TEST_F(ParserTest, ParseParametersMissingName)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT) };
    EXPECT_THROW(parser_.parse_parameters(), std::runtime_error);
}

// Test parse_function (int/void, params, body)
TEST_F(ParserTest, ParseFunctionSimple)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_VOID),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("func")),
        MakeToken(minic::TokenType::LPAREN),
        MakeToken(minic::TokenType::RPAREN),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::RBRACE) };
    auto func = parser_.parse_function();
    EXPECT_EQ(func->name, "func");
    EXPECT_EQ(func->return_type, minic::TokenType::KEYWORD_VOID);
    EXPECT_TRUE(func->parameters.empty());
    EXPECT_TRUE(func->body.empty());
}

TEST_F(ParserTest, ParseFunctionWithParamsAndBody)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("add")),
        MakeToken(minic::TokenType::LPAREN),
        MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("a")),
        MakeToken(minic::TokenType::COMMA),
        MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("b")),
        MakeToken(minic::TokenType::RPAREN),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("a")),
        MakeToken(minic::TokenType::OP_PLUS),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("b")),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE) };
    auto func = parser_.parse_function();
    EXPECT_EQ(func->name, "add");
    EXPECT_EQ(func->parameters.size(), 2);
    EXPECT_EQ(func->body.size(), 1);
}

TEST_F(ParserTest, ParseFunctionInvalidReturnType)
{
    tokens_ = { MakeToken(minic::TokenType::IDENTIFIER, std::string("bad")) };
    EXPECT_THROW(parser_.parse_function(), std::runtime_error);
}

TEST_F(ParserTest, ParseFunctionMissingParen)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("func")) };
    EXPECT_THROW(parser_.parse_function(), std::runtime_error);
}

TEST_F(ParserTest, ParseStatementInvalid)
{
    tokens_ = { MakeToken(minic::TokenType::OP_PLUS) };
    EXPECT_THROW(parser_.parse_statement(), std::runtime_error);
}

// Test full parse (program with multiple functions)
TEST_F(ParserTest, ParseProgramMultipleFunctions)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("main")),
        MakeToken(minic::TokenType::LPAREN),
        MakeToken(minic::TokenType::RPAREN),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::KEYWORD_RETURN),
        MakeToken(minic::TokenType::LITERAL_INT, 0),
        MakeToken(minic::TokenType::SEMICOLON),
        MakeToken(minic::TokenType::RBRACE),
        MakeToken(minic::TokenType::KEYWORD_VOID),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("test")),
        MakeToken(minic::TokenType::LPAREN),
        MakeToken(minic::TokenType::RPAREN),
        MakeToken(minic::TokenType::LBRACE),
        MakeToken(minic::TokenType::RBRACE) };
    auto program = parser_.parse();
    EXPECT_EQ(program->functions.size(), 2);
    EXPECT_EQ(program->functions[0]->name, "main");
    EXPECT_EQ(program->functions[1]->name, "test");
}

TEST_F(ParserTest, ParseProgramEmpty)
{
    auto program = parser_.parse();
    EXPECT_TRUE(program->functions.empty());
}

TEST_F(ParserTest, ParseProgramWithError)
{
    tokens_ = { MakeToken(minic::TokenType::KEYWORD_INT),
        MakeToken(minic::TokenType::IDENTIFIER, std::string("main")),
        MakeToken(minic::TokenType::LPAREN),
        MakeToken(minic::TokenType::RPAREN),
        MakeToken(minic::TokenType::LBRACE) };
    EXPECT_THROW(parser_.parse(), std::runtime_error); // Missing }
}

TEST_F(ParserTest, ParseFullProgram)
{
    std::string source = "int main() {\n"
                         "    int x = 5;\n"
                         "    x = x + 1;\n"
                         "    if (x > 0) {\n"
                         "        return x;\n"
                         "    } else {\n"
                         "        return 0;\n"
                         "    }\n"
                         "}";
    minic::Lexer lexer(source);
    tokens_ = lexer.Lex();
    EXPECT_NO_THROW(parser_.parse());
}

TEST_F(ParserTest, ParseComplexProgram)
{
    std::string source = "\n"
                         "int main() {\n"
                         "    int x = 5 + 3;\n"
                         "    if (x > 0) {\n"
                         "        while (x < 10) {\n"
                         "            x = x - 1;\n"
                         "        }\n"
                         "    }\n"
                         "    return x;\n"
                         "}\n";
    minic::Lexer lexer(source);
    tokens_ = lexer.Lex();
    EXPECT_NO_THROW(parser_.parse());
}