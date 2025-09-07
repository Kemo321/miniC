// minic/CodeGenerator.cpp
#include "minic/CodeGenerator.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace minic
{

CodeGenerator::CodeGenerator(std::ostream& out)
    : out_(&out)
    , type_map_({ { TokenType::KEYWORD_INT, "dq" },
          { TokenType::KEYWORD_VOID, "" },
          { TokenType::KEYWORD_STR, "db" } })
    , stack_offset_(0)
    , last_written_loc_("")
{
}

void CodeGenerator::generate(const IRProgram& ir_program, const std::string& output_file)
{
    std::ofstream file;
    std::ostream* previous_out = out_;
    std::cout << "[CodeGen] generate: output_file='" << output_file << "'\n";
    if (!output_file.empty())
    {
        file.open(output_file, std::ios::trunc);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open output file: " + output_file);
        }
        out_ = &file;
        std::cout << "[CodeGen] Writing to file: " << output_file << "\n";
    }
    else
    {
        std::cout << "[CodeGen] Writing to provided ostream\n";
    }

    (*out_) << "section .data\n";
    (*out_) << "section .text\n";
    (*out_) << "global _start\n";
    (*out_) << "_start:\n";
    (*out_) << "    call main\n";
    (*out_) << "    mov rdi, rax\n";
    (*out_) << "    mov rax, 60\n";
    (*out_) << "    syscall\n\n";

    std::cout << "[CodeGen] Emitting program\n";
    emit_program(ir_program);
    std::cout << "[CodeGen] Emission complete\n";

    out_->flush();
    if (!(*out_))
    {
        out_ = previous_out;
        throw std::runtime_error("Failed while writing to output stream/file.");
    }

    out_ = previous_out;
}

void CodeGenerator::emit_program(const IRProgram& program)
{
    std::cout << "[CodeGen] emit_program: function_count=" << program.functions.size() << "\n";
    for (const auto& func : program.functions)
    {
        emit_function(*func);
    }
}

void CodeGenerator::emit_function(const IRFunction& func)
{
    std::cout << "[CodeGen] emit_function: " << func.name << " params=" << func.parameters.size() << " blocks=" << func.blocks.size() << "\n";
    current_function_ = func.name;
    stack_offset_ = 0;
    var_offsets_.clear();
    block_labels_.clear();
    block_index_.clear();
    labels_.clear();
    last_written_loc_.clear();

    for (size_t i = 0; i < func.blocks.size(); ++i)
    {
        const std::string& lbl = func.blocks[i]->label;
        block_labels_.push_back(lbl);
        block_index_[lbl] = i;
        labels_.insert(lbl);
    }

    allocate_stack(func);

    std::cout << "[CodeGen] Function '" << func.name << "' stack_offset=" << stack_offset_ << " var_count=" << var_offsets_.size() << "\n";

    (*out_) << func.name << ":\n";
    (*out_) << "    push rbp\n";
    (*out_) << "    mov rbp, rsp\n";
    if (stack_offset_ > 0)
    {
        (*out_) << "    sub rsp, " << stack_offset_ << "\n";
    }

    const std::string param_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
    size_t param_idx = 0;
    for (const auto& param : func.parameters)
    {
        if (param_idx < 6)
        {
            (*out_) << "    mov [rbp - " << var_offsets_[param.name] << "], " << param_regs[param_idx] << "\n";
            std::cout << "[CodeGen] Param move: " << param.name << " <- " << param_regs[param_idx] << " offset=" << var_offsets_[param.name] << "\n";
        }
        param_idx++;
    }

    for (const auto& block : func.blocks)
    {
        emit_block(*block);
    }

    (*out_) << current_function_ << "_epilogue:\n";
    (*out_) << "    leave\n";
    (*out_) << "    ret\n\n";
    std::cout << "[CodeGen] Finished function: " << func.name << "\n";
}

void CodeGenerator::emit_block(const BasicBlock& block)
{
    current_block_label_ = block.label;
    std::cout << "[CodeGen] emit_block: " << block.label << " instructions=" << block.instructions.size() << "\n";
    (*out_) << block.label << ":\n";
    for (const auto& instr : block.instructions)
    {
        emit_instruction(instr);
    }

    if (!block.instructions.empty())
    {
        const IRInstruction& last = block.instructions.back();
        if (last.opcode != IROpcode::JUMP && last.opcode != IROpcode::JUMPIF && last.opcode != IROpcode::JUMPIFNOT && last.opcode != IROpcode::RETURN)
        {
            size_t idx = block_index_.at(current_block_label_);
            if (idx + 1 < block_labels_.size())
            {
                (*out_) << "    jmp " << block_labels_[idx + 1] << "\n";
                std::cout << "[CodeGen] Auto-jmp to " << block_labels_[idx + 1] << " from " << current_block_label_ << "\n";
            }
        }
    }
    else
    {
        size_t idx = block_index_.at(current_block_label_);
        if (idx + 1 < block_labels_.size())
        {
            (*out_) << "    jmp " << block_labels_[idx + 1] << "\n";
            std::cout << "[CodeGen] Empty block auto-jmp to " << block_labels_[idx + 1] << "\n";
        }
    }
}

void CodeGenerator::emit_instruction(const IRInstruction& instr)
{
    std::string res_loc = get_loc(instr.result);
    std::string op1_loc = get_loc(instr.operand1);
    std::string op2_loc = get_loc(instr.operand2);

    std::cout << "[CodeGen] emit_instruction: opcode=" << static_cast<int>(instr.opcode)
              << " result='" << instr.result << "' operand1='" << instr.operand1 << "' operand2='" << instr.operand2 << "'\n";
    std::cout << "[CodeGen] locations: res=" << res_loc << " op1=" << op1_loc << " op2=" << op2_loc << "\n";

    // For control flow instructions, handle specially if no explicit condition
    if ((instr.opcode == IROpcode::JUMPIF || instr.opcode == IROpcode::JUMPIFNOT) && (instr.operand1.empty() || op1_loc == "0"))
    {
        if (!last_written_loc_.empty())
        {
            op1_loc = last_written_loc_;
            std::cout << "[CodeGen] Using last_written_loc for condition: " << last_written_loc_ << "\n";
        }
    }

    switch (instr.opcode)
    {
    case IROpcode::ASSIGN:
        if (instr.operand1.find_first_not_of("0123456789") == std::string::npos)
        {
            // Literal assignment
            if (res_loc.find("[rbp") != std::string::npos)
                (*out_) << "    mov qword " << res_loc << ", " << instr.operand1 << "\n";
            else
                (*out_) << "    mov " << res_loc << ", " << instr.operand1 << "\n";
            std::cout << "[CodeGen] ASSIGN literal: " << instr.operand1 << " -> " << res_loc << "\n";
        }
        else
        {
            // Variable assignment
            (*out_) << "    mov rax, " << op1_loc << "\n";
            (*out_) << "    mov " << res_loc << ", rax\n";
            std::cout << "[CodeGen] ASSIGN var: " << op1_loc << " -> " << res_loc << "\n";
        }
        break;
    case IROpcode::ADD:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    add rax, " << op2_loc << "\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::SUB:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    sub rax, " << op2_loc << "\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::MUL:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    imul rax, " << op2_loc << "\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::DIV:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cqo\n";
        if (op2_loc.find_first_not_of("0123456789") == std::string::npos)
        {
            (*out_) << "    mov rbx, " << op2_loc << "\n";
            (*out_) << "    idiv rbx\n";
        }
        else
        {
            (*out_) << "    mov rbx, " << op2_loc << "\n";
            (*out_) << "    idiv rbx\n";
        }
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::NEG:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    neg rax\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::NOT:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    test rax, rax\n";
        (*out_) << "    setz al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::EQ:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    sete al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::NEQ:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    setne al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::LT:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    setl al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::GT:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    setg al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::LE:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    setle al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::GE:
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, " << op2_loc << "\n";
        (*out_) << "    setge al\n";
        (*out_) << "    movzx rax, al\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::JUMP:
    {
        std::string target = instr.operand1;
        if (target.empty())
            target = infer_target_label_for_current_block();
        if (target.empty())
        {
            (*out_) << "    ; missing jump target in " << current_function_ << " " << current_block_label_ << "\n";
            std::cout << "[CodeGen] JUMP: missing target in " << current_function_ << " " << current_block_label_ << "\n";
        }
        else
        {
            (*out_) << "    jmp " << target << "\n";
            std::cout << "[CodeGen] JUMP -> " << target << "\n";
        }
        break;
    }
    case IROpcode::JUMPIF:
    {
        std::string target = instr.operand2;
        if (target.empty())
            target = infer_target_label_for_current_block();
        if (target.empty())
        {
            (*out_) << "    mov rax, " << op1_loc << "\n";
            (*out_) << "    cmp rax, 0\n";
            (*out_) << "    ; missing jump target (JUMPIF) in " << current_function_ << " " << current_block_label_ << "\n";
            std::cout << "[CodeGen] JUMPIF: missing target, condition=" << op1_loc << "\n";
        }
        else
        {
            (*out_) << "    mov rax, " << op1_loc << "\n";
            (*out_) << "    cmp rax, 0\n";
            (*out_) << "    jne " << target << "\n";
            std::cout << "[CodeGen] JUMPIF -> " << target << " if " << op1_loc << " != 0\n";
        }
        break;
    }
    case IROpcode::JUMPIFNOT:
    {
        std::string target = instr.operand2;
        if (target.empty())
            target = infer_target_label_for_current_block();
        if (target.empty())
        {
            (*out_) << "    mov rax, " << op1_loc << "\n";
            (*out_) << "    cmp rax, 0\n";
            (*out_) << "    ; missing jump target (JUMPIFNOT) in " << current_function_ << " " << current_block_label_ << "\n";
            std::cout << "[CodeGen] JUMPIFNOT: missing target, condition=" << op1_loc << "\n";
        }
        else
        {
            (*out_) << "    mov rax, " << op1_loc << "\n";
            (*out_) << "    cmp rax, 0\n";
            (*out_) << "    je " << target << "\n";
            std::cout << "[CodeGen] JUMPIFNOT -> " << target << " if " << op1_loc << " == 0\n";
        }
        break;
    }
    case IROpcode::RETURN:
        if (!instr.operand1.empty())
        {
            (*out_) << "    mov rax, " << op1_loc << "\n";
            std::cout << "[CodeGen] RETURN value moved to rax: " << op1_loc << "\n";
        }
        (*out_) << "    jmp " << current_function_ << "_epilogue\n";
        std::cout << "[CodeGen] RETURN -> epilogue\n";
        break;
    default:
        throw std::runtime_error("Unsupported IR opcode in NASM codegen");
    }

    if (!instr.result.empty() && instr.opcode != IROpcode::JUMP && instr.opcode != IROpcode::JUMPIF && instr.opcode != IROpcode::JUMPIFNOT && instr.opcode != IROpcode::RETURN)
    {
        last_written_loc_ = res_loc;
        std::cout << "[CodeGen] last_written_loc updated to " << last_written_loc_ << "\n";
    }
}

std::string CodeGenerator::get_loc(const std::string& name)
{
    if (name.empty())
        return "0";
    if (name.find_first_not_of("0123456789") == std::string::npos)
        return name;
    if (labels_.count(name))
        return name;
    auto it = var_offsets_.find(name);
    if (it != var_offsets_.end())
        return "[rbp - " + std::to_string(it->second) + "]";
    // if unknown, allocate a slot for it now (ensures consistency)
    int newOff = stack_offset_ + 8;
    stack_offset_ = newOff;
    var_offsets_[name] = newOff;
    std::cout << "[CodeGen] get_loc: allocated new var '" << name << "' offset=" << newOff << " new stack_offset=" << stack_offset_ << "\n";
    return "[rbp - " + std::to_string(newOff) + "]";
}

std::string CodeGenerator::find_label_with_substr(const std::string& substr) const
{
    for (const auto& lbl : block_labels_)
    {
        if (lbl.find(substr) != std::string::npos)
            return lbl;
    }
    return "";
}

std::string CodeGenerator::infer_target_label_for_current_block() const
{
    if (current_block_label_.find("body") != std::string::npos)
    {
        std::string found = find_label_with_substr("cond");
        if (!found.empty())
        {
            std::cout << "[CodeGen] infer_target: body -> cond -> " << found << "\n";
            return found;
        }
    }
    size_t idx = block_index_.at(current_block_label_);
    if (idx + 1 < block_labels_.size())
    {
        std::cout << "[CodeGen] infer_target: next block -> " << block_labels_[idx + 1] << "\n";
        return block_labels_[idx + 1];
    }
    std::cout << "[CodeGen] infer_target: none found for block " << current_block_label_ << "\n";
    return "";
}

void CodeGenerator::allocate_stack(const IRFunction& func)
{
    std::cout << "[CodeGen] allocate_stack for " << func.name << "\n";
    std::unordered_set<std::string> all_vars;
    for (const auto& p : func.parameters)
        all_vars.insert(p.name);
    for (const auto& block : func.blocks)
    {
        for (const auto& instr : block->instructions)
        {
            if (!instr.result.empty() && labels_.count(instr.result) == 0)
                all_vars.insert(instr.result);
            if (!instr.operand1.empty() && instr.operand1.find_first_not_of("0123456789") != std::string::npos && labels_.count(instr.operand1) == 0)
                all_vars.insert(instr.operand1);
            if (!instr.operand2.empty() && instr.operand2.find_first_not_of("0123456789") != std::string::npos && labels_.count(instr.operand2) == 0)
                all_vars.insert(instr.operand2);
        }
    }

    std::vector<std::string> params;
    for (const auto& p : func.parameters)
        params.push_back(p.name);

    std::vector<std::string> locals;
    for (const auto& v : all_vars)
    {
        if (std::find(params.begin(), params.end(), v) == params.end())
            locals.push_back(v);
    }

    std::sort(locals.begin(), locals.end());

    int offset = 0;
    for (const auto& p : params)
    {
        std::cout << "Param: " << p << " Offset: " << (offset + 8) << "\n";
        offset += 8;
        var_offsets_[p] = offset;
    }
    for (const auto& v : locals)
    {
        std::cout << "Local: " << v << " Offset: " << (offset + 8) << "\n";
        offset += 8;
        var_offsets_[v] = offset;
    }

    stack_offset_ = offset;
    if (stack_offset_ % 16 != 0)
        stack_offset_ = ((stack_offset_ + 15) / 16) * 16;

    std::cout << "[CodeGen] allocate_stack done: final_stack_offset=" << stack_offset_ << " var_count=" << var_offsets_.size() << "\n";
}

} // namespace minic