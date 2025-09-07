#include "minic/IRGenerator.hpp"

namespace minic
{

std::unique_ptr<IRProgram> IRGenerator::generate(const Program& program)
{
    ir_program_ = std::make_unique<IRProgram>();
    visit(program);
    return std::move(ir_program_);
}

void IRGenerator::visit(const Program& program)
{
    for (const auto& func_ptr : program.functions)
    {
        if (func_ptr)
            visit(*func_ptr);
    }
}

void IRGenerator::visit(const Function& function)
{
    auto ir_func = std::make_unique<IRFunction>(function.name, function.return_type, function.parameters);
    current_function_ = ir_func.get();
    temp_counter_ = 0;
    label_counter_ = 0;
    var_map_.clear();

    auto entry_block = std::make_unique<BasicBlock>(new_label("entry"));
    current_block_ = entry_block.get();
    ir_func->blocks.push_back(std::move(entry_block));

    // Params (treat as vars)
    for (const auto& param : function.parameters)
    {
        var_map_[param.name] = param.name; // Use name directly
    }

    // Body
    for (const auto& stmt : function.body)
    {
        visit(*stmt);
    }

    ir_program_->functions.push_back(std::move(ir_func));
}

void IRGenerator::visit(const Stmt& stmt)
{
    if (auto* decl = dynamic_cast<const VarDeclStmt*>(&stmt))
    {
        std::string var = decl->name;
        var_map_[var] = var;
        if (decl->initializer)
        {
            std::string init_temp = generate_expr(*decl->initializer);
            emit(IROpcode::ASSIGN, var, init_temp);
        }
    }
    else if (auto* assign = dynamic_cast<const AssignStmt*>(&stmt))
    {
        std::string value_temp = generate_expr(*assign->value);
        emit(IROpcode::ASSIGN, assign->name, value_temp);
    }
    else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt))
    {
        if (ret->value)
        {
            std::string ret_temp = generate_expr(*ret->value);
            emit(IROpcode::RETURN, "", ret_temp);
        }
        else
        {
            emit(IROpcode::RETURN);
        }
    }
    else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt))
    {
        std::string cond_temp = generate_expr(*if_stmt->condition);
        std::string then_label = new_label("if_then");
        std::string else_label = new_label("if_else");
        std::string end_label = new_label("if_end");

        emit(IROpcode::JUMPIFNOT, "", cond_temp, else_label);

        // Then branch
        auto then_block = std::make_unique<BasicBlock>(then_label);
        current_block_ = then_block.get();
        current_function_->blocks.push_back(std::move(then_block));
        for (const auto& s : if_stmt->then_branch)
            visit(*s);
        emit(IROpcode::JUMP, "", end_label);

        // Else branch
        auto else_block = std::make_unique<BasicBlock>(else_label);
        current_block_ = else_block.get();
        current_function_->blocks.push_back(std::move(else_block));
        for (const auto& s : if_stmt->else_branch)
            visit(*s);
        emit(IROpcode::JUMP, "", end_label);

        // End
        auto end_block = std::make_unique<BasicBlock>(end_label);
        current_block_ = end_block.get();
        current_function_->blocks.push_back(std::move(end_block));
    }
    else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt))
    {
        std::string cond_label = new_label("while_cond");
        std::string body_label = new_label("while_body");
        std::string end_label = new_label("while_end");

        emit(IROpcode::JUMP, cond_label);

        // Cond block
        auto cond_block = std::make_unique<BasicBlock>(cond_label);
        current_block_ = cond_block.get();
        current_function_->blocks.push_back(std::move(cond_block));
        std::string cond_temp = generate_expr(*while_stmt->condition);
        emit(IROpcode::JUMPIFNOT, "", cond_temp, end_label); // Jump if false

        // Body block
        auto body_block = std::make_unique<BasicBlock>(body_label);
        current_block_ = body_block.get();
        current_function_->blocks.push_back(std::move(body_block));
        for (const auto& s : while_stmt->body)
            visit(*s);
        emit(IROpcode::JUMP, cond_label);

        // End block
        auto end_block = std::make_unique<BasicBlock>(end_label);
        current_block_ = end_block.get();
        current_function_->blocks.push_back(std::move(end_block));
    }
    else
    {
        throw std::runtime_error("Unsupported statement in IR generation");
    }
}

void IRGenerator::visit(const Expr& expr)
{
    generate_expr(expr); // Discard result if not used
}

std::string IRGenerator::generate_expr(const Expr& expr)
{
    if (auto* lit = dynamic_cast<const IntLiteral*>(&expr))
    {
        std::string temp = new_temp();
        emit(IROpcode::ASSIGN, temp, std::to_string(lit->value));
        return temp;
    }
    else if (auto* str_lit = dynamic_cast<const StringLiteral*>(&expr))
    {
        std::string temp = new_temp();
        emit(IROpcode::ASSIGN, temp, str_lit->value); // Assume string literals as constants
        return temp;
    }
    else if (auto* id = dynamic_cast<const Identifier*>(&expr))
    {
        auto it = var_map_.find(id->name);
        if (it == var_map_.end())
            throw std::runtime_error("Undeclared variable in IR");
        return it->second;
    }
    else if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
    {
        std::string oper_temp = generate_expr(*unary->operand);
        std::string result_temp = new_temp();
        IROpcode op = (unary->op == TokenType::OP_MINUS) ? IROpcode::NEG : IROpcode::NOT;
        emit(op, result_temp, oper_temp);
        return result_temp;
    }
    else if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr))
    {
        std::string left_temp = generate_expr(*bin->left);
        std::string right_temp = generate_expr(*bin->right);
        std::string result_temp = new_temp();
        IROpcode op;
        switch (bin->op)
        {
        case TokenType::OP_PLUS:
            op = IROpcode::ADD;
            break;
        case TokenType::OP_MINUS:
            op = IROpcode::SUB;
            break;
        case TokenType::OP_MULTIPLY:
            op = IROpcode::MUL;
            break;
        case TokenType::OP_DIVIDE:
            op = IROpcode::DIV;
            break;
        case TokenType::OP_EQUAL:
            op = IROpcode::EQ;
            break;
        case TokenType::OP_NOT_EQUAL:
            op = IROpcode::NEQ;
            break;
        case TokenType::OP_LESS:
            op = IROpcode::LT;
            break;
        case TokenType::OP_GREATER:
            op = IROpcode::GT;
            break;
        case TokenType::OP_LESS_EQ:
            op = IROpcode::LE;
            break;
        case TokenType::OP_GREATER_EQ:
            op = IROpcode::GE;
            break;
        default:
            throw std::runtime_error("Unsupported binary operator in IR");
        }
        emit(op, result_temp, left_temp, right_temp);
        return result_temp;
    }
    else
    {
        throw std::runtime_error("Unsupported expression in IR generation");
    }
}

std::string IRGenerator::new_temp()
{
    return "t" + std::to_string(temp_counter_++);
}

std::string IRGenerator::new_label(const std::string& prefix)
{
    return prefix + "_" + std::to_string(label_counter_++);
}

void IRGenerator::emit(IROpcode op, const std::string& res, const std::string& op1, const std::string& op2)
{
    current_block_->instructions.emplace_back(op, res, op1, op2);
}

} // namespace minic