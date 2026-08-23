#include "minic/SemanticAnalyzer.hpp"

namespace minic
{

SemanticAnalyzer::SemanticAnalyzer()
{
    push_scope(); // Global scope

    // Built-in: print(int) — lowered to printf by the code generator
    FunctionInfo print_info;
    print_info.return_type = TokenType::KEYWORD_VOID;
    print_info.param_types.push_back(TokenType::KEYWORD_INT);
    functions_["print"] = std::move(print_info);
}

SemanticAnalyzer::~SemanticAnalyzer() = default;

void SemanticAnalyzer::visit(const Program& program)
{
    // Collect function signatures; reject redefinitions
    for (const auto& func : program.functions)
    {
        if (functions_.find(func->name) != functions_.end())
        {
            throw SemanticError("Function '" + func->name + "' redefined");
        }
        FunctionInfo info;
        info.return_type = func->return_type;
        for (const auto& param : func->parameters)
        {
            info.param_types.push_back(param.type);
        }
        functions_[func->name] = std::move(info);
    }

    for (const auto& func : program.functions)
    {
        current_function_type_ = func->return_type;
        loop_depth_ = 0;
        push_scope(); // New scope for each function
        visit(*func);
        pop_scope();
    }
}

void SemanticAnalyzer::visit(const Function& function)
{
    // Parameter declarations
    for (const auto& param : function.parameters)
    {
        if (is_declared_in_current_scope(param.name))
        {
            throw SemanticError("Parameter '" + param.name + "' redeclared");
        }
        scopes_.top()[param.name] = param.type;
    }

    // Function body
    for (const auto& stmt : function.body)
    {
        visit(*stmt);
    }
}

void SemanticAnalyzer::visit(const Stmt& stmt)
{
    if (auto* expr_stmt = dynamic_cast<const ExprStmt*>(&stmt))
    {
        visit(*expr_stmt->expression);
    }
    else if (auto* decl = dynamic_cast<const VarDeclStmt*>(&stmt))
    {
        if (is_declared_in_current_scope(decl->name))
        {
            throw SemanticError("Variable '" + decl->name + "' redeclared in current scope");
        }
        if (decl->type == TokenType::KEYWORD_VOID)
        {
            throw SemanticError("Cannot declare variable '" + decl->name + "' as void");
        }
        scopes_.top()[decl->name] = decl->type;
        if (decl->initializer)
        {
            visit(*decl->initializer);
            TokenType init_type = infer_type(*decl->initializer);
            if (init_type != decl->type)
            {
                throw SemanticError("Type mismatch in declaration of '" + decl->name + "': expected " + std::to_string(static_cast<int>(decl->type)) + ", got " + std::to_string(static_cast<int>(init_type)));
            }
        }
    }
    else if (auto* assign = dynamic_cast<const AssignStmt*>(&stmt))
    {
        TokenType var_type = get_type(assign->name);
        if (var_type == TokenType::KEYWORD_VOID)
        {
            throw SemanticError("Cannot assign to void variable '" + assign->name + "'");
        }
        visit(*assign->value);
        TokenType value_type = infer_type(*assign->value);
        if (var_type != value_type)
        {
            throw SemanticError("Type mismatch in assignment to '" + assign->name + "': expected " + std::to_string(static_cast<int>(var_type)) + ", got " + std::to_string(static_cast<int>(value_type)));
        }
    }
    else if (auto* dassign = dynamic_cast<const DerefAssignStmt*>(&stmt))
    {
        visit(*dassign->target);
        visit(*dassign->value);
        TokenType ptr_type = infer_type(*dassign->target);
        if (ptr_type != TokenType::TYPE_PTR_INT)
        {
            throw SemanticError("Dereference assignment requires a pointer-to-int target");
        }
        TokenType value_type = infer_type(*dassign->value);
        if (value_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Dereference assignment requires an int value");
        }
    }
    else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt))
    {
        if (ret->value)
        {
            visit(*ret->value);
            TokenType ret_type = infer_type(*ret->value);
            if (ret_type != current_function_type_)
            {
                throw SemanticError("Return type mismatch: expected " + std::to_string(static_cast<int>(current_function_type_)) + ", got " + std::to_string(static_cast<int>(ret_type)));
            }
        }
        else
        {
            if (current_function_type_ != TokenType::KEYWORD_VOID)
            {
                throw SemanticError("Non-void function must return a value");
            }
        }
    }
    else if (dynamic_cast<const BreakStmt*>(&stmt))
    {
        if (loop_depth_ <= 0)
        {
            throw SemanticError("'break' used outside of a loop");
        }
    }
    else if (dynamic_cast<const ContinueStmt*>(&stmt))
    {
        if (loop_depth_ <= 0)
        {
            throw SemanticError("'continue' used outside of a loop");
        }
    }
    else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt))
    {
        visit(*if_stmt->condition);
        TokenType cond_type = infer_type(*if_stmt->condition);
        if (cond_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("If condition must be int type, got " + std::to_string(static_cast<int>(cond_type)));
        }
        push_scope();
        for (const auto& s : if_stmt->then_branch)
        {
            visit(*s);
        }
        pop_scope();
        push_scope();
        for (const auto& s : if_stmt->else_branch)
        {
            visit(*s);
        }
        pop_scope();
    }
    else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt))
    {
        visit(*while_stmt->condition);
        TokenType cond_type = infer_type(*while_stmt->condition);
        if (cond_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("While condition must be int type, got " + std::to_string(static_cast<int>(cond_type)));
        }
        ++loop_depth_;
        push_scope();
        for (const auto& s : while_stmt->body)
        {
            visit(*s);
        }
        pop_scope();
        --loop_depth_;
    }
    else
    {
        throw SemanticError("Unknown statement type");
    }
}

void SemanticAnalyzer::visit(const Expr& expr)
{
    if (auto* id = dynamic_cast<const Identifier*>(&expr))
    {
        get_type(id->name); // Throws if undeclared
    }
    else if (auto* call = dynamic_cast<const CallExpr*>(&expr))
    {
        auto it = functions_.find(call->callee);
        if (it == functions_.end())
        {
            throw SemanticError("Call to undeclared function '" + call->callee + "'");
        }
        const FunctionInfo& info = it->second;
        if (call->arguments.size() != info.param_types.size())
        {
            throw SemanticError("Function '" + call->callee + "' expects " + std::to_string(info.param_types.size())
                + " argument(s), but " + std::to_string(call->arguments.size()) + " were provided");
        }
        for (size_t i = 0; i < call->arguments.size(); ++i)
        {
            visit(*call->arguments[i]);
            TokenType arg_type = infer_type(*call->arguments[i]);
            if (arg_type != info.param_types[i])
            {
                throw SemanticError("Argument " + std::to_string(i + 1) + " of '" + call->callee
                    + "' has wrong type: expected " + std::to_string(static_cast<int>(info.param_types[i]))
                    + ", got " + std::to_string(static_cast<int>(arg_type)));
            }
            if (arg_type == TokenType::KEYWORD_VOID)
            {
                throw SemanticError("Cannot pass void value as argument " + std::to_string(i + 1) + " to '" + call->callee + "'");
            }
        }
    }
    else if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
    {
        visit(*unary->operand);
        TokenType oper_type = infer_type(*unary->operand);
        if (oper_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Unary operator requires int operand");
        }
    }
    else if (auto* addr = dynamic_cast<const AddressOfExpr*>(&expr))
    {
        visit(*addr->operand);
        auto* id = dynamic_cast<const Identifier*>(addr->operand.get());
        if (!id)
        {
            throw SemanticError("Address-of operator requires a variable operand");
        }
        TokenType oper_type = infer_type(*addr->operand);
        if (oper_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Address-of is only supported for int variables");
        }
    }
    else if (auto* deref = dynamic_cast<const DereferenceExpr*>(&expr))
    {
        visit(*deref->operand);
        TokenType oper_type = infer_type(*deref->operand);
        if (oper_type != TokenType::TYPE_PTR_INT)
        {
            throw SemanticError("Dereference operator requires a pointer-to-int operand");
        }
    }
    else if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr))
    {
        visit(*bin->left);
        visit(*bin->right);
        TokenType left_type = infer_type(*bin->left);
        TokenType right_type = infer_type(*bin->right);
        validate_binary_op(bin->op, left_type, right_type);
    }
    else if (dynamic_cast<const IntLiteral*>(&expr) || dynamic_cast<const StringLiteral*>(&expr))
    {
        // Literals are fine
    }
    else
    {
        throw SemanticError("Unknown expression type");
    }
}

void SemanticAnalyzer::push_scope()
{
    scopes_.push(SymbolTable());
}

void SemanticAnalyzer::pop_scope()
{
    if (scopes_.empty())
    {
        throw SemanticError("Scope stack underflow");
    }
    scopes_.pop();
}

bool SemanticAnalyzer::is_declared_in_current_scope(const std::string& name) const
{
    if (scopes_.empty())
        return false;
    return scopes_.top().find(name) != scopes_.top().end();
}

bool SemanticAnalyzer::is_declared(const std::string& name) const
{
    std::stack<SymbolTable> copy = scopes_;
    while (!copy.empty())
    {
        const auto& table = copy.top();
        if (table.find(name) != table.end())
            return true;
        copy.pop();
    }
    return false;
}

TokenType SemanticAnalyzer::get_type(const std::string& name) const
{
    std::stack<SymbolTable> copy = scopes_;
    while (!copy.empty())
    {
        const auto& table = copy.top();
        auto var_it = table.find(name);
        if (var_it != table.end())
            return var_it->second;
        copy.pop();
    }
    throw SemanticError("Variable '" + name + "' not declared");
}

TokenType SemanticAnalyzer::infer_type(const Expr& expr)
{
    if (dynamic_cast<const IntLiteral*>(&expr))
    {
        return TokenType::KEYWORD_INT;
    }
    else if (dynamic_cast<const StringLiteral*>(&expr))
    {
        return TokenType::KEYWORD_STR;
    }
    else if (auto* id = dynamic_cast<const Identifier*>(&expr))
    {
        return get_type(id->name);
    }
    else if (auto* call = dynamic_cast<const CallExpr*>(&expr))
    {
        auto it = functions_.find(call->callee);
        if (it == functions_.end())
        {
            throw SemanticError("Call to undeclared function '" + call->callee + "'");
        }
        return it->second.return_type;
    }
    else if (auto* unary = dynamic_cast<const UnaryExpr*>(&expr))
    {
        TokenType oper_type = infer_type(*unary->operand);
        if (oper_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Type inference failed for unary expression");
        }
        return TokenType::KEYWORD_INT;
    }
    else if (auto* addr = dynamic_cast<const AddressOfExpr*>(&expr))
    {
        TokenType oper_type = infer_type(*addr->operand);
        if (oper_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Address-of requires an int variable");
        }
        return TokenType::TYPE_PTR_INT;
    }
    else if (auto* deref = dynamic_cast<const DereferenceExpr*>(&expr))
    {
        TokenType oper_type = infer_type(*deref->operand);
        if (oper_type != TokenType::TYPE_PTR_INT)
        {
            throw SemanticError("Dereference requires a pointer-to-int");
        }
        return TokenType::KEYWORD_INT;
    }
    else if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr))
    {
        TokenType left_type = infer_type(*bin->left);
        TokenType right_type = infer_type(*bin->right);
        if (left_type == TokenType::KEYWORD_INT && right_type == TokenType::KEYWORD_INT)
        {
            return TokenType::KEYWORD_INT;
        }
        else
        {
            throw SemanticError("Type inference failed for binary expression");
        }
    }
    else
    {
        throw SemanticError("Cannot infer type for unknown expression");
    }
}

void SemanticAnalyzer::validate_binary_op(TokenType op, TokenType left_type, TokenType right_type)
{
    bool is_arithmetic = (op == TokenType::OP_PLUS || op == TokenType::OP_MINUS || op == TokenType::OP_MULTIPLY || op == TokenType::OP_DIVIDE);
    bool is_comparison = (op == TokenType::OP_EQUAL || op == TokenType::OP_NOT_EQUAL || op == TokenType::OP_LESS || op == TokenType::OP_LESS_EQ || op == TokenType::OP_GREATER || op == TokenType::OP_GREATER_EQ);

    if (is_arithmetic || is_comparison)
    {
        if (left_type != TokenType::KEYWORD_INT || right_type != TokenType::KEYWORD_INT)
        {
            throw SemanticError("Operands for operator must be int");
        }
    }
    else
    {
        throw SemanticError("Unsupported binary operator");
    }
}

} // namespace minic