#include "minic/IRGenerator.hpp"
#include "minic/Parser.hpp"
#include <gtest/gtest.h>

namespace minic
{

// PublicIRGenerator exposes private members for testing
class PublicIRGenerator : public IRGenerator
{
public:
    using IRGenerator::current_block_;
    using IRGenerator::current_function_;
    using IRGenerator::emit;
    using IRGenerator::generate_expr;
    using IRGenerator::ir_program_;
    using IRGenerator::label_counter_;
    using IRGenerator::new_label;
    using IRGenerator::new_temp;
    using IRGenerator::temp_counter_;
    using IRGenerator::var_map_;
};

// Helper to build AST nodes for testing
std::unique_ptr<Program> BuildProgram(std::vector<std::unique_ptr<Function>> funcs)
{
    return std::make_unique<Program>(std::move(funcs));
}

std::unique_ptr<Function> BuildFunction(const std::string& name, TokenType ret_type, std::vector<Parameter> params, std::vector<std::unique_ptr<Stmt>> body)
{
    return std::make_unique<Function>(name, ret_type, std::move(params), std::move(body));
}

std::unique_ptr<VarDeclStmt> BuildVarDecl(TokenType type, const std::string& name, std::unique_ptr<Expr> init = nullptr)
{
    return std::make_unique<VarDeclStmt>(type, name, std::move(init));
}

std::unique_ptr<AssignStmt> BuildAssign(const std::string& name, std::unique_ptr<Expr> value)
{
    return std::make_unique<AssignStmt>(name, std::move(value));
}

std::unique_ptr<ReturnStmt> BuildReturn(std::unique_ptr<Expr> value = nullptr)
{
    return std::make_unique<ReturnStmt>(std::move(value));
}

std::unique_ptr<IntLiteral> BuildIntLit(int val)
{
    return std::make_unique<IntLiteral>(val);
}

std::unique_ptr<StringLiteral> BuildStrLit(const std::string& val)
{
    return std::make_unique<StringLiteral>(val);
}

std::unique_ptr<Identifier> BuildId(const std::string& name)
{
    return std::make_unique<Identifier>(name);
}

std::unique_ptr<UnaryExpr> BuildUnary(TokenType op, std::unique_ptr<Expr> operand)
{
    return std::make_unique<UnaryExpr>(op, std::move(operand));
}

std::unique_ptr<BinaryExpr> BuildBinary(std::unique_ptr<Expr> left, TokenType op, std::unique_ptr<Expr> right)
{
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
}

std::unique_ptr<IfStmt> BuildIf(std::unique_ptr<Expr> cond, std::vector<std::unique_ptr<Stmt>> then_b, std::vector<std::unique_ptr<Stmt>> else_b)
{
    return std::make_unique<IfStmt>(std::move(cond), std::move(then_b), std::move(else_b));
}

std::unique_ptr<WhileStmt> BuildWhile(std::unique_ptr<Expr> cond, std::vector<std::unique_ptr<Stmt>> body)
{
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

class IRGeneratorTest : public ::testing::Test
{
protected:
    minic::PublicIRGenerator generator_;

    // Helper to parse source to AST
    std::unique_ptr<minic::Program> ParseSource(const std::string& source)
    {
        minic::Lexer lexer(source);
        auto tokens = lexer.Lex();
        minic::Parser parser(tokens);
        return parser.parse();
    }

    // Helper to get instruction count in a block
    size_t CountInstructions(const minic::BasicBlock* block)
    {
        return block ? block->instructions.size() : 0;
    }

    // Helper to check if instruction exists in block
    bool HasInstruction(const minic::BasicBlock* block, minic::IROpcode op, const std::string& res = "", const std::string& op1 = "", const std::string& op2 = "")
    {
        if (!block)
            return false;
        for (const auto& instr : block->instructions)
        {
            if (instr.opcode == op && (res.empty() || instr.result == res) && (op1.empty() || instr.operand1 == op1) && (op2.empty() || instr.operand2 == op2))
            {
                return true;
            }
        }
        return false;
    }

    // Helper to find block by label prefix
    const minic::BasicBlock* FindBlockByLabelPrefix(const minic::IRFunction* func, const std::string& prefix)
    {
        for (const auto& block : func->blocks)
        {
            if (block->label.find(prefix) == 0)
            {
                return block.get();
            }
        }
        return nullptr;
    }
};

TEST_F(IRGeneratorTest, DeclWithInit)
{
    auto init = minic::BuildBinary(minic::BuildIntLit(5), TokenType::OP_PLUS, minic::BuildIntLit(3));
    auto decl = minic::BuildVarDecl(TokenType::KEYWORD_INT, "x", std::move(init));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    ASSERT_EQ(ir->functions.size(), 1);
    ASSERT_EQ(ir->functions[0]->blocks.size(), 1);
    const auto* entry = ir->functions[0]->blocks[0].get();
    ASSERT_EQ(CountInstructions(entry), 4); // ASSIGN t0=5, ASSIGN t1=3, ADD t2=t0+t1, ASSIGN x=t2
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "", "5"));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "", "3"));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ADD));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "x"));
}

TEST_F(IRGeneratorTest, DeclNoInit)
{
    auto decl = minic::BuildVarDecl(TokenType::KEYWORD_INT, "x");
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(decl));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_EQ(CountInstructions(entry), 0);
}

TEST_F(IRGeneratorTest, AssignComplexExpr)
{
    auto expr = minic::BuildBinary(
        minic::BuildUnary(TokenType::OP_MINUS, minic::BuildId("y")),
        TokenType::OP_MULTIPLY,
        minic::BuildBinary(minic::BuildIntLit(2), TokenType::OP_DIVIDE, minic::BuildIntLit(4)));
    auto assign = minic::BuildAssign("x", std::move(expr));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(minic::BuildVarDecl(TokenType::KEYWORD_INT, "y"));
    body.push_back(std::move(assign));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_GE(CountInstructions(entry), 6); // NEG, DIV, MUL, assigns
    EXPECT_TRUE(HasInstruction(entry, IROpcode::NEG));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::DIV));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::MUL));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "x"));
}

TEST_F(IRGeneratorTest, ReturnIntLiteral)
{
    auto ret = minic::BuildReturn(minic::BuildIntLit(42));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(ret));

    auto func = minic::BuildFunction("func", TokenType::KEYWORD_INT, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_EQ(CountInstructions(entry), 2);
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "", "42"));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::RETURN));
}

TEST_F(IRGeneratorTest, ReturnVoid)
{
    auto ret = minic::BuildReturn();
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(ret));

    auto func = minic::BuildFunction("func", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_EQ(CountInstructions(entry), 1);
    EXPECT_TRUE(HasInstruction(entry, IROpcode::RETURN));
}

TEST_F(IRGeneratorTest, IfWithBranches)
{
    auto cond = minic::BuildBinary(minic::BuildId("x"), TokenType::OP_GREATER, minic::BuildIntLit(0));
    auto then_assign = minic::BuildAssign("y", minic::BuildIntLit(1));
    std::vector<std::unique_ptr<minic::Stmt>> then_b;
    then_b.push_back(std::move(then_assign));
    auto else_assign = minic::BuildAssign("y", minic::BuildIntLit(0));
    std::vector<std::unique_ptr<minic::Stmt>> else_b;
    else_b.push_back(std::move(else_assign));
    auto if_stmt = minic::BuildIf(std::move(cond), std::move(then_b), std::move(else_b));

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(minic::BuildVarDecl(TokenType::KEYWORD_INT, "x"));
    body.push_back(minic::BuildVarDecl(TokenType::KEYWORD_INT, "y"));
    body.push_back(std::move(if_stmt));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions[0]->blocks.size(), 4);
    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry, IROpcode::GT));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::JUMPIFNOT));

    const auto* then_block = FindBlockByLabelPrefix(ir->functions[0].get(), "if_then");
    EXPECT_TRUE(HasInstruction(then_block, IROpcode::ASSIGN, "", "1"));
    EXPECT_TRUE(HasInstruction(then_block, IROpcode::ASSIGN, "y"));
    EXPECT_TRUE(HasInstruction(then_block, IROpcode::JUMP));

    const auto* else_block = FindBlockByLabelPrefix(ir->functions[0].get(), "if_else");
    EXPECT_TRUE(HasInstruction(else_block, IROpcode::ASSIGN, "", "0"));
    EXPECT_TRUE(HasInstruction(else_block, IROpcode::ASSIGN, "y"));
    EXPECT_TRUE(HasInstruction(else_block, IROpcode::JUMP));

    const auto* end_block = FindBlockByLabelPrefix(ir->functions[0].get(), "if_end");
    EXPECT_EQ(CountInstructions(end_block), 0);
}

TEST_F(IRGeneratorTest, IfNoElse)
{
    auto cond = minic::BuildIntLit(1);
    auto then_assign = minic::BuildAssign("x", minic::BuildIntLit(1));
    std::vector<std::unique_ptr<minic::Stmt>> then_b;
    then_b.push_back(std::move(then_assign));
    auto if_stmt = minic::BuildIf(std::move(cond), std::move(then_b), {});

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(if_stmt));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions[0]->blocks.size(), 4);
    EXPECT_TRUE(FindBlockByLabelPrefix(ir->functions[0].get(), "if_then"));
    EXPECT_TRUE(FindBlockByLabelPrefix(ir->functions[0].get(), "if_else"));
    EXPECT_TRUE(FindBlockByLabelPrefix(ir->functions[0].get(), "if_end"));
}

TEST_F(IRGeneratorTest, WhileWithBody)
{
    auto cond = minic::BuildBinary(minic::BuildId("i"), TokenType::OP_LESS, minic::BuildIntLit(10));
    auto inc = minic::BuildAssign("i", minic::BuildBinary(minic::BuildId("i"), TokenType::OP_PLUS, minic::BuildIntLit(1)));
    std::vector<std::unique_ptr<minic::Stmt>> loop_body;
    loop_body.push_back(std::move(inc));
    auto while_stmt = minic::BuildWhile(std::move(cond), std::move(loop_body));

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(minic::BuildVarDecl(TokenType::KEYWORD_INT, "i"));
    body.push_back(std::move(while_stmt));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions[0]->blocks.size(), 4);
    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry, IROpcode::JUMP));

    const auto* cond_block = FindBlockByLabelPrefix(ir->functions[0].get(), "while_cond");
    EXPECT_TRUE(HasInstruction(cond_block, IROpcode::LT));
    EXPECT_TRUE(HasInstruction(cond_block, IROpcode::JUMPIFNOT));

    const auto* body_block = FindBlockByLabelPrefix(ir->functions[0].get(), "while_body");
    EXPECT_TRUE(HasInstruction(body_block, IROpcode::ADD));
    EXPECT_TRUE(HasInstruction(body_block, IROpcode::ASSIGN, "i"));
    EXPECT_TRUE(HasInstruction(body_block, IROpcode::JUMP));

    const auto* end_block = FindBlockByLabelPrefix(ir->functions[0].get(), "while_end");
    EXPECT_EQ(CountInstructions(end_block), 0);
}

TEST_F(IRGeneratorTest, WhileNoBody)
{
    auto cond = minic::BuildIntLit(0);
    auto while_stmt = minic::BuildWhile(std::move(cond), {});

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(while_stmt));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions[0]->blocks.size(), 4);
    const auto* body_block = FindBlockByLabelPrefix(ir->functions[0].get(), "while_body");
    EXPECT_EQ(CountInstructions(body_block), 1); // JUMP to cond
}

TEST_F(IRGeneratorTest, WhileFalseCond)
{
    auto cond = minic::BuildIntLit(0);
    auto while_stmt = minic::BuildWhile(std::move(cond), {});

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(while_stmt));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* cond_block = FindBlockByLabelPrefix(ir->functions[0].get(), "while_cond");
    EXPECT_TRUE(HasInstruction(cond_block, IROpcode::ASSIGN, "", "0"));
    EXPECT_TRUE(HasInstruction(cond_block, IROpcode::JUMPIFNOT));
}

TEST_F(IRGeneratorTest, UnaryOps)
{
    // Unary minus
    auto unary = minic::BuildUnary(TokenType::OP_MINUS, minic::BuildIntLit(10));
    auto assign = minic::BuildAssign("x", std::move(unary));
    std::vector<std::unique_ptr<minic::Stmt>> body1;
    body1.push_back(std::move(assign));
    auto func1 = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body1));
    std::vector<std::unique_ptr<minic::Function>> funcs1;
    funcs1.push_back(std::move(func1));
    auto ir1 = generator_.generate(*minic::BuildProgram(std::move(funcs1)));
    const auto* entry1 = ir1->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry1, IROpcode::ASSIGN, "", "10"));
    EXPECT_TRUE(HasInstruction(entry1, IROpcode::NEG));
    EXPECT_TRUE(HasInstruction(entry1, IROpcode::ASSIGN, "x"));

    // Unary not
    generator_ = minic::PublicIRGenerator(); // reset
    auto unary_not = minic::BuildUnary(TokenType::OP_NOT, minic::BuildIntLit(0));
    auto ret = minic::BuildReturn(std::move(unary_not));
    std::vector<std::unique_ptr<minic::Stmt>> body2;
    body2.push_back(std::move(ret));
    auto func2 = minic::BuildFunction("func", TokenType::KEYWORD_INT, {}, std::move(body2));
    std::vector<std::unique_ptr<minic::Function>> funcs2;
    funcs2.push_back(std::move(func2));
    auto ir2 = generator_.generate(*minic::BuildProgram(std::move(funcs2)));
    const auto* entry2 = ir2->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry2, IROpcode::ASSIGN, "", "0"));
    EXPECT_TRUE(HasInstruction(entry2, IROpcode::NOT));
    EXPECT_TRUE(HasInstruction(entry2, IROpcode::RETURN));
}

TEST_F(IRGeneratorTest, StringAssign)
{
    auto assign = minic::BuildAssign("s", minic::BuildStrLit("hello"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(assign));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "", "hello"));
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "s"));
}

TEST_F(IRGeneratorTest, ParamUsageAndVarMapParam)
{
    Parameter p1(TokenType::KEYWORD_INT, "a");
    std::vector<Parameter> params;
    params.push_back(p1);
    auto assign = minic::BuildAssign("b", minic::BuildId("a"));
    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(assign));

    auto func = minic::BuildFunction("func", TokenType::KEYWORD_VOID, std::move(params), std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    const auto* entry = ir->functions[0]->blocks[0].get();
    EXPECT_TRUE(HasInstruction(entry, IROpcode::ASSIGN, "b", "a"));

    {
        Parameter p(TokenType::KEYWORD_INT, "p");
        std::vector<Parameter> ps;
        ps.push_back(p);
        auto f = minic::BuildFunction("test", TokenType::KEYWORD_VOID, std::move(ps), {});

        generator_.ir_program_ = std::make_unique<minic::IRProgram>();
        generator_.visit(*f);
        EXPECT_EQ(generator_.var_map_["p"], "p");
    }
}

TEST_F(IRGeneratorTest, EmptyFunctionAndFunctionNoBody)
{
    auto func = minic::BuildFunction("empty", TokenType::KEYWORD_VOID, {}, {});
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));
    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);
    EXPECT_EQ(ir->functions[0]->blocks.size(), 1);
    EXPECT_EQ(CountInstructions(ir->functions[0]->blocks[0].get()), 0);

    auto f2 = minic::BuildFunction("no_body", TokenType::KEYWORD_VOID, {}, {});
    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    generator_.visit(*f2);
    EXPECT_EQ(generator_.current_function_->blocks.size(), 1);
    EXPECT_EQ(CountInstructions(generator_.current_function_->blocks[0].get()), 0);
}

TEST_F(IRGeneratorTest, NestedIfWhile)
{
    auto inner_cond = minic::BuildIntLit(1);
    auto inner_assign = minic::BuildAssign("inner", minic::BuildIntLit(3));
    std::vector<std::unique_ptr<minic::Stmt>> inner_body;
    inner_body.push_back(std::move(inner_assign));
    auto inner_while = minic::BuildWhile(std::move(inner_cond), std::move(inner_body));

    std::vector<std::unique_ptr<minic::Stmt>> then_branch;
    then_branch.push_back(std::move(inner_while));
    auto outer_cond = minic::BuildIntLit(1);
    auto outer_if = minic::BuildIf(std::move(outer_cond), std::move(then_branch), {});

    std::vector<std::unique_ptr<minic::Stmt>> body;
    body.push_back(std::move(outer_if));

    auto func = minic::BuildFunction("main", TokenType::KEYWORD_VOID, {}, std::move(body));
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_GE(ir->functions[0]->blocks.size(), 7);
    EXPECT_TRUE(FindBlockByLabelPrefix(ir->functions[0].get(), "if_then"));
    EXPECT_TRUE(FindBlockByLabelPrefix(ir->functions[0].get(), "while_cond"));
}

TEST_F(IRGeneratorTest, ErrorCases)
{
    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    auto irf = std::make_unique<minic::IRFunction>("err_test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    irf->blocks.push_back(std::make_unique<minic::BasicBlock>("entry"));
    generator_.current_function_ = irf.get();
    generator_.current_block_ = irf->blocks[0].get();
    generator_.ir_program_->functions.push_back(std::move(irf));

    class UnknownStmt : public minic::Stmt
    {
    };
    auto unknown = std::make_unique<UnknownStmt>();
    EXPECT_THROW(generator_.visit(*unknown), std::runtime_error);

    class UnknownExpr : public minic::Expr
    {
    };
    auto unknownE = std::make_unique<UnknownExpr>();
    EXPECT_THROW(generator_.generate_expr(*unknownE), std::runtime_error);

    auto bin = std::make_unique<minic::BinaryExpr>(minic::BuildIntLit(1), TokenType::OP_ASSIGN, minic::BuildIntLit(2));
    EXPECT_THROW(generator_.generate_expr(*bin), std::runtime_error);
}

TEST_F(IRGeneratorTest, TempLabelAndCounters)
{
    generator_.temp_counter_ = 0;
    EXPECT_EQ(generator_.new_temp(), "t0");
    EXPECT_EQ(generator_.new_temp(), "t1");

    generator_.label_counter_ = 0;
    EXPECT_EQ(generator_.new_label("test"), "test_0");
    EXPECT_EQ(generator_.new_label("test"), "test_1");

    generator_.temp_counter_ = 5;
    EXPECT_EQ(generator_.new_temp(), "t5");
    EXPECT_EQ(generator_.temp_counter_, 6);

    generator_.label_counter_ = 3;
    EXPECT_EQ(generator_.new_label("p"), "p_3");
    EXPECT_EQ(generator_.label_counter_, 4);
}

TEST_F(IRGeneratorTest, EmitInstructionAndVariations)
{
    auto block = std::make_unique<minic::BasicBlock>("test");
    generator_.current_block_ = block.get();
    generator_.emit(minic::IROpcode::ADD, "t2", "t0", "t1");
    EXPECT_EQ(CountInstructions(block.get()), 1);
    EXPECT_EQ(block->instructions[0].opcode, minic::IROpcode::ADD);
    EXPECT_EQ(block->instructions[0].result, "t2");
    EXPECT_EQ(block->instructions[0].operand1, "t0");
    EXPECT_EQ(block->instructions[0].operand2, "t1");

    // Full
    generator_.emit(minic::IROpcode::ADD, "res", "op1", "op2");
    EXPECT_EQ(block->instructions.back().result, "res");

    // No res
    generator_.emit(minic::IROpcode::JUMP, "", "label");
    EXPECT_EQ(block->instructions.back().operand1, "label");
    EXPECT_TRUE(block->instructions.back().result.empty());

    // No op2
    generator_.emit(minic::IROpcode::NEG, "res", "op1");
    EXPECT_TRUE(block->instructions.back().operand2.empty());

    // No op1/op2
    generator_.emit(minic::IROpcode::RETURN);
    EXPECT_TRUE(block->instructions.back().operand1.empty());
    EXPECT_TRUE(block->instructions.back().operand2.empty());
}

TEST_F(IRGeneratorTest, VarMapAndIRProgram)
{
    generator_.var_map_["var"] = "t10";
    EXPECT_EQ(generator_.var_map_["var"], "t10");

    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    EXPECT_TRUE(generator_.ir_program_ != nullptr);
}

TEST_F(IRGeneratorTest, MultipleFunctions)
{
    auto func1 = minic::BuildFunction("func1", TokenType::KEYWORD_VOID, {}, {});
    auto func2 = minic::BuildFunction("func2", TokenType::KEYWORD_INT, {}, {});
    std::vector<std::unique_ptr<minic::Function>> funcs;
    funcs.push_back(std::move(func1));
    funcs.push_back(std::move(func2));

    auto ast = minic::BuildProgram(std::move(funcs));
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions.size(), 2);
    EXPECT_EQ(ir->functions[0]->name, "func1");
    EXPECT_EQ(ir->functions[1]->name, "func2");
}

TEST_F(IRGeneratorTest, FullProgramViaParser)
{
    std::string source = "\n"
                         "int main() {\n"
                         "    int x = 5;\n"
                         "    x = x + 1;\n        if (x > 0) {\n"
                         "        return -x;\n"
                         "    } else {\n"
                         "        return !x;\n"
                         "    }\n"
                         "}\n";
    auto ast = ParseSource(source);
    auto ir = generator_.generate(*ast);

    EXPECT_EQ(ir->functions.size(), 1);
    EXPECT_GE(ir->functions[0]->blocks.size(), 4);
    const auto* then_block = FindBlockByLabelPrefix(ir->functions[0].get(), "if_then");
    EXPECT_TRUE(HasInstruction(then_block, IROpcode::NEG));
    const auto* else_block = FindBlockByLabelPrefix(ir->functions[0].get(), "if_else");
    EXPECT_TRUE(HasInstruction(else_block, IROpcode::NOT));
}

TEST_F(IRGeneratorTest, AllArithmeticOps)
{
    struct TestCase
    {
        TokenType token_op;
        minic::IROpcode ir_op;
    };
    std::vector<TestCase> cases = {
        { TokenType::OP_PLUS, IROpcode::ADD },
        { TokenType::OP_MINUS, IROpcode::SUB },
        { TokenType::OP_MULTIPLY, IROpcode::MUL },
        { TokenType::OP_DIVIDE, IROpcode::DIV }
    };

    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    auto irf = std::make_unique<minic::IRFunction>("arith_test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    irf->blocks.push_back(std::make_unique<minic::BasicBlock>("entry"));
    generator_.current_function_ = irf.get();
    generator_.current_block_ = irf->blocks[0].get();
    generator_.ir_program_->functions.push_back(std::move(irf));

    generator_.temp_counter_ = 0;

    for (const auto& tc : cases)
    {
        auto bin = minic::BuildBinary(minic::BuildIntLit(10), tc.token_op, minic::BuildIntLit(2));
        generator_.generate_expr(*bin);
    }
    EXPECT_GE(generator_.temp_counter_, cases.size() * 3);
}

TEST_F(IRGeneratorTest, AllComparisonOps)
{
    struct TestCase
    {
        TokenType token_op;
        minic::IROpcode ir_op;
    };
    std::vector<TestCase> cases = {
        { TokenType::OP_EQUAL, IROpcode::EQ },
        { TokenType::OP_NOT_EQUAL, IROpcode::NEQ },
        { TokenType::OP_LESS, IROpcode::LT },
        { TokenType::OP_GREATER, IROpcode::GT },
        { TokenType::OP_LESS_EQ, IROpcode::LE },
        { TokenType::OP_GREATER_EQ, IROpcode::GE }
    };

    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    auto irf = std::make_unique<minic::IRFunction>("cmp_test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    irf->blocks.push_back(std::make_unique<minic::BasicBlock>("entry"));
    generator_.current_function_ = irf.get();
    generator_.current_block_ = irf->blocks[0].get();
    generator_.ir_program_->functions.push_back(std::move(irf));

    generator_.temp_counter_ = 0;

    for (const auto& tc : cases)
    {
        auto bin = minic::BuildBinary(minic::BuildIntLit(10), tc.token_op, minic::BuildIntLit(2));
        generator_.generate_expr(*bin);
    }
    EXPECT_GE(generator_.temp_counter_, cases.size() * 3);
}

// Id not mapped error
TEST_F(IRGeneratorTest, IdNotMappedError)
{
    auto id = minic::BuildId("missing");
    EXPECT_THROW(generator_.generate_expr(*id), std::runtime_error);
}

TEST_F(IRGeneratorTest, MultipleEmitsAndNestedExpr)
{
    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    auto irf = std::make_unique<minic::IRFunction>("nested_test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    irf->blocks.push_back(std::make_unique<minic::BasicBlock>("entry"));
    generator_.current_function_ = irf.get();
    generator_.current_block_ = irf->blocks[0].get();
    generator_.ir_program_->functions.push_back(std::move(irf));

    generator_.temp_counter_ = 0;

    auto nested = minic::BuildBinary(
        minic::BuildBinary(minic::BuildIntLit(1), TokenType::OP_PLUS, minic::BuildIntLit(2)),
        TokenType::OP_MULTIPLY,
        minic::BuildIntLit(3));
    generator_.generate_expr(*nested);
    EXPECT_EQ(generator_.temp_counter_, 5); // ASSIGNs for 1,2,3 + ADD + MUL
}

TEST_F(IRGeneratorTest, ExprDiscardAndVisit)
{
    auto expr = minic::BuildIntLit(5);

    generator_.ir_program_ = std::make_unique<minic::IRProgram>();
    auto irf = std::make_unique<minic::IRFunction>("expr_test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    irf->blocks.push_back(std::make_unique<minic::BasicBlock>("entry"));
    generator_.current_function_ = irf.get();
    generator_.current_block_ = irf->blocks[0].get();
    generator_.ir_program_->functions.push_back(std::move(irf));

    int prev_temp = generator_.temp_counter_;
    generator_.visit(*expr);
    EXPECT_EQ(generator_.temp_counter_, prev_temp + 1);
}

TEST_F(IRGeneratorTest, PrivateCurrentPointers)
{
    auto block = std::make_unique<minic::BasicBlock>("private");
    generator_.current_block_ = block.get();
    EXPECT_EQ(generator_.current_block_->label, "private");

    auto func = std::make_unique<minic::IRFunction>("test", TokenType::KEYWORD_VOID, std::vector<minic::Parameter> {});
    generator_.current_function_ = func.get();
    EXPECT_EQ(generator_.current_function_->name, "test");
}

}