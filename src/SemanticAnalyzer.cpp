#include "miniC/SemanticAnalyzer.hpp"

namespace minic
{
SemanticAnalyzer::SemanticAnalyzer() = default;

SemanticAnalyzer::~SemanticAnalyzer() = default;

bool SemanticAnalyzer::SymbolTable::is_declared(const std::string& name) const
{
    return variables.find(name) != variables.end();
}

TokenType SemanticAnalyzer::SymbolTable::get_type(const std::string& name) const
{
    auto it = variables.find(name);
    if (it == variables.end())
    {
        throw std::runtime_error("SymbolTable: '" + name + "' not found");
    }
    return it->second;
}

void SemanticAnalyzer::visit(const Program& program)
{
    for (const auto& func : program.functions)
    {
        current_scope_ = SymbolTable();
        visit(*func);
    }
}

void SemanticAnalyzer::visit(const Function& function)
{
    // Parameter declarations
    for (const auto& param : function.parameters)
    {
        if (current_scope_.is_declared(param.name))
        {
            throw std::runtime_error("Parameter '" + param.name + "' redeclared");
        }
        current_scope_.variables[param.name] = param.type;
    }

    // Function body
    for (const auto& stmt : function.body)
    {
        visit(*stmt);
    }
}

void SemanticAnalyzer::visit(const Stmt& stmt)
{
    if (auto* decl = dynamic_cast<const VarDeclStmt*>(&stmt))
    {
        if (current_scope_.is_declared(decl->name))
        {
            throw std::runtime_error("Variable '" + decl->name + "' redeclared");
        }
        current_scope_.variables[decl->name] = decl->type;
        if (decl->initializer)
        {
            visit(*decl->initializer);
        }
    }
    else if (auto* assign = dynamic_cast<const AssignStmt*>(&stmt))
    {
        if (!current_scope_.is_declared(assign->name))
        {
            throw std::runtime_error("Variable '" + assign->name + "' not declared");
        }
        visit(*assign->value);
    }
    else if (auto* ret = dynamic_cast<const ReturnStmt*>(&stmt))
    {
        if (ret->value)
        {
            visit(*ret->value);
        }
    }
    else if (auto* if_stmt = dynamic_cast<const IfStmt*>(&stmt))
    {
        visit(*if_stmt->condition);
        for (const auto& s : if_stmt->then_branch)
        {
            visit(*s);
        }
        for (const auto& s : if_stmt->else_branch)
        {
            visit(*s);
        }
    }
    else if (auto* while_stmt = dynamic_cast<const WhileStmt*>(&stmt))
    {
        visit(*while_stmt->condition);
        for (const auto& s : while_stmt->body)
        {
            visit(*s);
        }
    }
    else
    {
        throw std::runtime_error("Unknown statement type");
    }
}

void SemanticAnalyzer::visit(const Expr& expr)
{
    if (auto* id = dynamic_cast<const Identifier*>(&expr))
    {
        if (!current_scope_.is_declared(id->name))
        {
            throw std::runtime_error("Variable '" + id->name + "' not declared");
        }
    }
    else if (auto* bin = dynamic_cast<const BinaryExpr*>(&expr))
    {
        visit(*bin->left);
        visit(*bin->right);
    }
    else if (dynamic_cast<const IntLiteral*>(&expr) || dynamic_cast<const StringLiteral*>(&expr))
    {
        // Literals are fine
    }
    else
    {
        throw std::runtime_error("Unknown expression type");
    }
}

} // namespace minic
