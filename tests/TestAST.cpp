#include "minic/AST.hpp"
#include <gtest/gtest.h>

using namespace minic;

class ASTNodeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code if needed
    }
    void TearDown() override
    {
        // Cleanup code if needed
    }
};

TEST(ASTNodeTest, IntLiteral)
{
    IntLiteral lit(42);
    EXPECT_EQ(lit.value, 42);
}

TEST(ASTNodeTest, StringLiteral)
{
    StringLiteral lit("hello");
    EXPECT_EQ(lit.value, "hello");
}

TEST(ASTNodeTest, Identifier)
{
    Identifier id("var");
    EXPECT_EQ(id.name, "var");
}

TEST(ASTNodeTest, BinaryExpr)
{
    auto left = std::make_unique<IntLiteral>(1);
    auto right = std::make_unique<IntLiteral>(2);
    BinaryExpr expr(std::move(left), TokenType::OP_PLUS, std::move(right));
    EXPECT_EQ(expr.op, TokenType::OP_PLUS);
    EXPECT_EQ(static_cast<IntLiteral*>(expr.left.get())->value, 1);
    EXPECT_EQ(static_cast<IntLiteral*>(expr.right.get())->value, 2);
}

TEST(ASTNodeTest, ReturnStmt)
{
    auto val = std::make_unique<IntLiteral>(99);
    ReturnStmt ret(std::move(val));
    EXPECT_EQ(static_cast<IntLiteral*>(ret.value.get())->value, 99);
}

TEST(ASTNodeTest, IfStmtBranches)
{
    auto cond = std::make_unique<Identifier>("cond");
    std::vector<std::unique_ptr<Stmt>> then_branch;
    then_branch.push_back(std::make_unique<ReturnStmt>(std::make_unique<IntLiteral>(1)));
    std::vector<std::unique_ptr<Stmt>> else_branch;
    else_branch.push_back(std::make_unique<ReturnStmt>(std::make_unique<IntLiteral>(0)));
    IfStmt ifstmt(std::move(cond), std::move(then_branch), std::move(else_branch));
    EXPECT_EQ(static_cast<Identifier*>(ifstmt.condition.get())->name, "cond");
    EXPECT_EQ(ifstmt.then_branch.size(), 1);
    EXPECT_EQ(ifstmt.else_branch.size(), 1);
}

TEST(ASTNodeTest, WhileStmtBody)
{
    auto cond = std::make_unique<IntLiteral>(1);
    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::make_unique<ReturnStmt>(std::make_unique<IntLiteral>(2)));
    WhileStmt whilestmt(std::move(cond), std::move(body));
    EXPECT_EQ(static_cast<IntLiteral*>(whilestmt.condition.get())->value, 1);
    EXPECT_EQ(whilestmt.body.size(), 1);
}

TEST(ASTNodeTest, AssignStmt)
{
    auto val = std::make_unique<IntLiteral>(123);
    AssignStmt assign("x", std::move(val));
    EXPECT_EQ(assign.name, "x");
    EXPECT_EQ(static_cast<IntLiteral*>(assign.value.get())->value, 123);
}

TEST(ASTNodeTest, Parameter)
{
    Parameter param(TokenType::KEYWORD_INT, "foo");
    EXPECT_EQ(param.type, TokenType::KEYWORD_INT);
    EXPECT_EQ(param.name, "foo");
}

TEST(ASTNodeTest, Function)
{
    std::vector<Parameter> params = { Parameter(TokenType::KEYWORD_INT, "x") };
    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::make_unique<ReturnStmt>(std::make_unique<IntLiteral>(5)));
    Function func("f", TokenType::KEYWORD_INT, params, std::move(body));
    EXPECT_EQ(func.name, "f");
    EXPECT_EQ(func.return_type, TokenType::KEYWORD_INT);
    ASSERT_EQ(func.parameters.size(), 1);
    EXPECT_EQ(func.parameters[0].name, "x");
    ASSERT_EQ(func.body.size(), 1);
    EXPECT_EQ(static_cast<IntLiteral*>(static_cast<ReturnStmt*>(func.body[0].get())->value.get())->value, 5);
}

TEST(ASTNodeTest, Program)
{
    std::vector<Parameter> params = { Parameter(TokenType::KEYWORD_INT, "x") };
    std::vector<std::unique_ptr<Stmt>> body;
    body.push_back(std::make_unique<ReturnStmt>(std::make_unique<IntLiteral>(7)));
    auto func = std::make_unique<Function>("main", TokenType::KEYWORD_INT, params, std::move(body));
    std::vector<std::unique_ptr<Function>> funcs;
    funcs.push_back(std::move(func));
    Program prog(std::move(funcs));
    ASSERT_EQ(prog.functions.size(), 1);
    EXPECT_EQ(prog.functions[0]->name, "main");
}