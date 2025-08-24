#include "miniC/Parser.hpp" // For parsing full programs in some tests
#include "miniC/SemanticAnalyzer.hpp"
#include <gtest/gtest.h>

namespace minic
{

// Helper to build minimal AST for isolated testing
std::unique_ptr<Program> BuildSimpleProgram(std::vector<std::unique_ptr<Function>> funcs)
{
    return std::make_unique<Program>(std::move(funcs));
}

std::unique_ptr<Function> BuildFunction(const std::string& name, TokenType ret_type,
    std::vector<Parameter> params,
    std::vector<std::unique_ptr<Stmt>> body)
{
    return std::make_unique<Function>(name, ret_type, std::move(params), std::move(body));
}

} // namespace minic

class SemanticAnalyzerTest : public ::testing::Test
{
protected:
    minic::SemanticAnalyzer analyzer_;

    // Helper to parse a source string into Program (uses Lexer/Parser)
    std::unique_ptr<minic::Program> ParseSource(const std::string& source)
    {
        minic::Lexer lexer(source);
        auto tokens = lexer.Lex();
        minic::Parser parser(tokens);
        return parser.parse();
    }
};

TEST_F(SemanticAnalyzerTest, ValidDeclAndAssign)
{
    auto decl = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_INT, "x",
        std::make_unique<minic::IntLiteral>(5));
    auto assign = std::make_unique<minic::AssignStmt>("x",
        std::make_unique<minic::BinaryExpr>(
            std::make_unique<minic::Identifier>("x"),
            minic::TokenType::OP_PLUS,
            std::make_unique<minic::IntLiteral>(1)));
    auto ret = std::make_unique<minic::ReturnStmt>(std::make_unique<minic::Identifier>("x"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl));
    body.push_back(std::move(assign));
    body.push_back(std::move(ret));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, UndeclaredAssign)
{
    auto assign = std::make_unique<minic::AssignStmt>("x", std::make_unique<minic::IntLiteral>(5));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(assign));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, RedeclaredVariable)
{
    auto decl1 = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_INT, "x");
    auto decl2 = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_INT, "x");
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl1));
    body.push_back(std::move(decl2));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, RedeclaredParameter)
{
    minic::Parameter param1(minic::TokenType::KEYWORD_INT, "a");
    minic::Parameter param2(minic::TokenType::KEYWORD_INT, "a");
    std::vector<minic::Parameter> params = { param1, param2 };

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("func", minic::TokenType::KEYWORD_VOID, std::move(params), {}));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, ValidIfStatement)
{
    auto decl = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_INT, "x",
        std::make_unique<minic::IntLiteral>(1));
    auto cond = std::make_unique<minic::BinaryExpr>(std::make_unique<minic::Identifier>("x"),
        minic::TokenType::OP_GREATER,
        std::make_unique<minic::IntLiteral>(0));
    std::vector<std::unique_ptr<minic::Stmt>> then_branch;
    then_branch.push_back(std::make_unique<minic::ReturnStmt>(std::make_unique<minic::IntLiteral>(1)));
    std::vector<std::unique_ptr<minic::Stmt>> else_branch;
    else_branch.push_back(std::make_unique<minic::ReturnStmt>(std::make_unique<minic::IntLiteral>(0)));
    auto if_stmt = std::make_unique<minic::IfStmt>(std::move(cond), std::move(then_branch), std::move(else_branch));

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl));
    body.push_back(std::move(if_stmt));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, UndeclaredInIfCondition)
{
    auto cond = std::make_unique<minic::Identifier>("undeclared");
    auto if_stmt = std::make_unique<minic::IfStmt>(
        std::move(cond),
        std::vector<std::unique_ptr<minic::Stmt>> {},
        std::vector<std::unique_ptr<minic::Stmt>> {});

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(if_stmt));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, ValidWhileLoop)
{
    auto decl = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_INT, "i",
        std::make_unique<minic::IntLiteral>(0));
    auto cond = std::make_unique<minic::BinaryExpr>(std::make_unique<minic::Identifier>("i"),
        minic::TokenType::OP_LESS,
        std::make_unique<minic::IntLiteral>(10));
    std::vector<std::unique_ptr<minic::Stmt>> loop_body;
    loop_body.push_back(std::make_unique<minic::AssignStmt>("i",
        std::make_unique<minic::BinaryExpr>(
            std::make_unique<minic::Identifier>("i"),
            minic::TokenType::OP_PLUS,
            std::make_unique<minic::IntLiteral>(1))));
    auto while_stmt = std::make_unique<minic::WhileStmt>(std::move(cond), std::move(loop_body));

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl));
    body.push_back(std::move(while_stmt));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, UndeclaredInWhileBody)
{
    auto cond = std::make_unique<minic::IntLiteral>(1);
    std::vector<std::unique_ptr<minic::Stmt>> loop_body;
    loop_body.push_back(std::make_unique<minic::AssignStmt>("undeclared", std::make_unique<minic::IntLiteral>(0)));
    auto while_stmt = std::make_unique<minic::WhileStmt>(std::move(cond), std::move(loop_body));

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(while_stmt));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, ValidReturnLiteral)
{
    auto ret = std::make_unique<minic::ReturnStmt>(std::make_unique<minic::StringLiteral>("hello"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(ret));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("func", minic::TokenType::KEYWORD_VOID, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, UndeclaredInReturn)
{
    auto ret = std::make_unique<minic::ReturnStmt>(std::make_unique<minic::Identifier>("undeclared"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(ret));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, ValidParameters)
{
    minic::Parameter param1(minic::TokenType::KEYWORD_STR, "s");
    minic::Parameter param2(minic::TokenType::KEYWORD_INT, "n");
    std::vector<minic::Parameter> params = { param1, param2 };

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("func", minic::TokenType::KEYWORD_VOID, std::move(params), {}));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, UnknownStmtType)
{
    class UnknownStmt : public minic::Stmt
    {
    }; // Mock unknown
    auto unknown = std::make_unique<UnknownStmt>();
    EXPECT_THROW(analyzer_.visit(*unknown), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, UnknownExprType)
{
    class UnknownExpr : public minic::Expr
    {
    }; // Mock unknown
    auto unknown = std::make_unique<UnknownExpr>();
    EXPECT_THROW(analyzer_.visit(*unknown), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, FullValidProgramViaParser)
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
    auto program = ParseSource(source);
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, FullInvalidProgramViaParser)
{
    std::string source = "int main() {\n"
                         "    x = 5;  // Undeclared\n"
                         "}\n";
    auto program = ParseSource(source);
    EXPECT_THROW(analyzer_.visit(*program), std::runtime_error);
}

TEST_F(SemanticAnalyzerTest, VoidStringDecl)
{
    auto decl_void = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_VOID, "v");
    auto decl_str = std::make_unique<minic::VarDeclStmt>(minic::TokenType::KEYWORD_STR, "s",
        std::make_unique<minic::StringLiteral>("test"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl_void));
    body.push_back(std::move(decl_str));

    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("main", minic::TokenType::KEYWORD_INT, {}, std::move(body)));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program)); // Passes, as no type rules enforced yet
}

TEST_F(SemanticAnalyzerTest, EmptyProgram)
{
    auto program = minic::BuildSimpleProgram({});
    EXPECT_NO_THROW(analyzer_.visit(*program));
}

TEST_F(SemanticAnalyzerTest, MultipleFunctions)
{
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(minic::BuildFunction("func1", minic::TokenType::KEYWORD_VOID, {}, {}));
    funcs.push_back(minic::BuildFunction("func2", minic::TokenType::KEYWORD_INT, {}, {}));

    auto program = minic::BuildSimpleProgram(std::move(funcs));
    EXPECT_NO_THROW(analyzer_.visit(*program)); // No cross-function checks yet
}