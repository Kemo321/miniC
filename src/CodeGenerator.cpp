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

bool CodeGenerator::is_ir_temp(const std::string& name)
{
    if (name.size() < 2 || name[0] != 't')
        return false;
    return name.find_first_not_of("0123456789", 1) == std::string::npos;
}

void CodeGenerator::reset_register_pool()
{
    free_regs_.clear();
    for (size_t i = 0; i < kTempRegisterCount; ++i)
        free_regs_.emplace_back(kTempRegisters[i]);
    // Pop from the back → prefer r10, then r11, ...
    std::reverse(free_regs_.begin(), free_regs_.end());
}

void CodeGenerator::plan_temporary_registers(const IRFunction& func)
{
    temp_to_reg_.clear();
    spilled_temps_.clear();
    must_spill_temps_.clear();
    temp_remaining_uses_.clear();
    reset_register_pool();

    struct Interval
    {
        int first_def = -1;
        int last_use = -1;
    };
    std::unordered_map<std::string, Interval> intervals;

    auto note_def = [&](const std::string& name, int idx)
    {
        if (!is_ir_temp(name))
            return;
        auto& iv = intervals[name];
        if (iv.first_def < 0)
            iv.first_def = idx;
    };
    auto note_use = [&](const std::string& name, int idx)
    {
        if (!is_ir_temp(name))
            return;
        intervals[name].last_use = idx;
        ++temp_remaining_uses_[name];
    };

    std::vector<int> call_sites;
    int idx = 0;
    for (const auto& block : func.blocks)
    {
        for (const auto& instr : block->instructions)
        {
            if (instr.opcode == IROpcode::CALL)
                call_sites.push_back(idx);

            if (!instr.result.empty())
                note_def(instr.result, idx);

            switch (instr.opcode)
            {
            case IROpcode::JUMP:
                break;
            case IROpcode::JUMPIF:
            case IROpcode::JUMPIFNOT:
                note_use(instr.operand1, idx);
                break;
            case IROpcode::CALL:
                for (const auto& arg : instr.args)
                    note_use(arg, idx);
                break;
            default:
                note_use(instr.operand1, idx);
                note_use(instr.operand2, idx);
                for (const auto& arg : instr.args)
                    note_use(arg, idx);
                break;
            }
            ++idx;
        }
    }

    for (const auto& [name, iv] : intervals)
    {
        if (temp_remaining_uses_.find(name) == temp_remaining_uses_.end())
            temp_remaining_uses_[name] = 0;

        if (iv.first_def < 0)
            continue;
        const int last = (iv.last_use >= 0) ? iv.last_use : iv.first_def;
        for (int call_idx : call_sites)
        {
            if (iv.first_def < call_idx && last > call_idx)
            {
                must_spill_temps_.insert(name);
                break;
            }
        }
    }

    // Snapshot of use counts for the emit phase (planning walk mutates a copy)
    const std::unordered_map<std::string, int> uses_for_emit = temp_remaining_uses_;
    std::unordered_map<std::string, int> uses = temp_remaining_uses_;

    // Record final assignment separately because release returns regs to the pool
    std::unordered_map<std::string, std::string> final_regs;
    std::unordered_set<std::string> final_spills;

    auto ensure_final = [&](const std::string& name)
    {
        if (!is_ir_temp(name))
            return;
        if (final_regs.count(name) || final_spills.count(name))
            return;

        if (!must_spill_temps_.count(name) && !free_regs_.empty())
        {
            std::string reg = free_regs_.back();
            free_regs_.pop_back();
            final_regs[name] = reg;
            temp_to_reg_[name] = reg;
            std::cout << "[CodeGen] plan: " << name << " -> " << reg << "\n";
        }
        else
        {
            final_spills.insert(name);
            spilled_temps_.insert(name);
            std::cout << "[CodeGen] plan: " << name << " -> spill"
                      << (must_spill_temps_.count(name) ? " (live across CALL)" : " (pool exhausted)") << "\n";
        }
    };

    auto release_final = [&](const std::string& name)
    {
        if (!is_ir_temp(name))
            return;
        auto uit = uses.find(name);
        if (uit == uses.end())
            return;
        if (uit->second > 0)
            --(uit->second);
        if (uit->second != 0)
            return;

        auto reg_it = temp_to_reg_.find(name);
        if (reg_it == temp_to_reg_.end())
            return;

        free_regs_.push_back(reg_it->second);
        std::cout << "[CodeGen] plan: last use of " << name << ", free " << reg_it->second << "\n";
        temp_to_reg_.erase(reg_it);
        // final_regs keeps the chosen register for this temp's lifetime
    };

    temp_to_reg_.clear();
    spilled_temps_.clear();
    reset_register_pool();

    for (const auto& block : func.blocks)
    {
        for (const auto& instr : block->instructions)
        {
            if (!instr.result.empty())
                ensure_final(instr.result);

            switch (instr.opcode)
            {
            case IROpcode::JUMP:
                break;
            case IROpcode::JUMPIF:
            case IROpcode::JUMPIFNOT:
                ensure_final(instr.operand1);
                release_final(instr.operand1);
                break;
            case IROpcode::CALL:
                for (const auto& arg : instr.args)
                {
                    ensure_final(arg);
                    release_final(arg);
                }
                break;
            default:
                ensure_final(instr.operand1);
                ensure_final(instr.operand2);
                release_final(instr.operand1);
                release_final(instr.operand2);
                for (const auto& arg : instr.args)
                {
                    ensure_final(arg);
                    release_final(arg);
                }
                break;
            }

            if (is_ir_temp(instr.result))
            {
                auto uit = uses.find(instr.result);
                if (uit != uses.end() && uit->second == 0)
                    release_final(instr.result);
            }
        }
    }

    temp_to_reg_ = std::move(final_regs);
    spilled_temps_ = std::move(final_spills);
    temp_remaining_uses_ = uses_for_emit;
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
    (*out_) << "fmt_int: db \"%d\", 10, 0\n"; // "%d\\n" for built-in print(int)
    (*out_) << "\n";
    (*out_) << "section .text\n";
    (*out_) << "extern printf\n";
    (*out_) << "global main\n\n";

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
    known_functions_.clear();
    known_functions_.insert("print");
    known_functions_.insert("printf");
    for (const auto& func : program.functions)
    {
        known_functions_.insert(func->name);
    }
    for (const auto& func : program.functions)
    {
        emit_function(*func);
    }
}

void CodeGenerator::emit_function(const IRFunction& func)
{
    std::cout << "[CodeGen] emit_function: " << func.name << " params=" << func.parameters.size()
              << " blocks=" << func.blocks.size() << "\n";
    current_function_ = func.name;
    stack_offset_ = 0;
    var_offsets_.clear();
    block_labels_.clear();
    block_index_.clear();
    labels_.clear();
    last_written_loc_.clear();
    temp_to_reg_.clear();
    spilled_temps_.clear();
    must_spill_temps_.clear();
    temp_remaining_uses_.clear();

    for (size_t i = 0; i < func.blocks.size(); ++i)
    {
        const std::string& lbl = func.blocks[i]->label;
        block_labels_.push_back(lbl);
        block_index_[lbl] = i;
        labels_.insert(lbl);
    }

    plan_temporary_registers(func);
    allocate_stack(func);

    // Emit-time free list: all pool regs start free; consume_temp returns them on last use.
    // get_loc reads the planned assignment in temp_to_reg_ / spilled_temps_.
    reset_register_pool();

    std::cout << "[CodeGen] Function '" << func.name << "' stack_offset=" << stack_offset_
              << " var_count=" << var_offsets_.size() << " reg_temps=" << temp_to_reg_.size()
              << " spilled_temps=" << spilled_temps_.size() << "\n";

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
            std::cout << "[CodeGen] Param move: " << param.name << " <- " << param_regs[param_idx]
                      << " offset=" << var_offsets_[param.name] << "\n";
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
        if (last.opcode != IROpcode::JUMP && last.opcode != IROpcode::JUMPIF && last.opcode != IROpcode::JUMPIFNOT
            && last.opcode != IROpcode::RETURN)
        {
            size_t bidx = block_index_.at(current_block_label_);
            if (bidx + 1 < block_labels_.size())
            {
                (*out_) << "    jmp " << block_labels_[bidx + 1] << "\n";
                std::cout << "[CodeGen] Auto-jmp to " << block_labels_[bidx + 1] << " from " << current_block_label_ << "\n";
            }
        }
    }
    else
    {
        size_t bidx = block_index_.at(current_block_label_);
        if (bidx + 1 < block_labels_.size())
        {
            (*out_) << "    jmp " << block_labels_[bidx + 1] << "\n";
            std::cout << "[CodeGen] Empty block auto-jmp to " << block_labels_[bidx + 1] << "\n";
        }
    }
}

std::string CodeGenerator::allocate_temp_location(const std::string& name)
{
    // Prefer planned register assignment
    auto reg_it = temp_to_reg_.find(name);
    if (reg_it != temp_to_reg_.end())
        return reg_it->second;

    if (spilled_temps_.count(name))
    {
        auto off = var_offsets_.find(name);
        if (off != var_offsets_.end())
            return "[rbp - " + std::to_string(off->second) + "]";
    }

    // Online fallback if planning missed a temp
    if (!must_spill_temps_.count(name) && !free_regs_.empty())
    {
        std::string reg = free_regs_.back();
        free_regs_.pop_back();
        temp_to_reg_[name] = reg;
        std::cout << "[CodeGen] allocate_temp (online): " << name << " -> " << reg << "\n";
        return reg;
    }

    spilled_temps_.insert(name);
    auto off = var_offsets_.find(name);
    if (off == var_offsets_.end())
    {
        int newOff = stack_offset_ + 8;
        stack_offset_ = newOff;
        if (stack_offset_ % 16 != 0)
            stack_offset_ = ((stack_offset_ + 15) / 16) * 16;
        var_offsets_[name] = newOff;
        std::cout << "[CodeGen] allocate_temp (online spill): " << name << " offset=" << newOff << "\n";
        return "[rbp - " + std::to_string(newOff) + "]";
    }
    return "[rbp - " + std::to_string(off->second) + "]";
}

void CodeGenerator::consume_temp(const std::string& name)
{
    if (!is_ir_temp(name))
        return;

    auto it = temp_remaining_uses_.find(name);
    if (it == temp_remaining_uses_.end())
        return;

    if (it->second > 0)
        --(it->second);

    if (it->second != 0)
        return;

    auto reg_it = temp_to_reg_.find(name);
    if (reg_it == temp_to_reg_.end())
        return;

    // Return register to the pool; keep mapping so get_loc stays consistent if
    // referenced again incorrectly, but mark as freed via free_regs_ only once.
    if (std::find(free_regs_.begin(), free_regs_.end(), reg_it->second) == free_regs_.end())
    {
        free_regs_.push_back(reg_it->second);
        std::cout << "[CodeGen] last use of " << name << ", freed " << reg_it->second << "\n";
    }
}

void CodeGenerator::emit_instruction(const IRInstruction& instr)
{
    std::vector<std::string> arg_locs;
    arg_locs.reserve(instr.args.size());
    for (const auto& arg : instr.args)
        arg_locs.push_back(get_loc(arg));

    std::string res_loc = get_loc(instr.result);
    std::string op1_loc = get_loc(instr.operand1);
    std::string op2_loc = get_loc(instr.operand2);

    std::cout << "[CodeGen] emit_instruction: opcode=" << static_cast<int>(instr.opcode)
              << " result='" << instr.result << "' operand1='" << instr.operand1 << "' operand2='" << instr.operand2 << "'\n";
    std::cout << "[CodeGen] locations: res=" << res_loc << " op1=" << op1_loc << " op2=" << op2_loc << "\n";

    if ((instr.opcode == IROpcode::JUMPIF || instr.opcode == IROpcode::JUMPIFNOT)
        && (instr.operand1.empty() || op1_loc == "0"))
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
            if (res_loc.find("[rbp") != std::string::npos)
                (*out_) << "    mov qword " << res_loc << ", " << instr.operand1 << "\n";
            else
                (*out_) << "    mov " << res_loc << ", " << instr.operand1 << "\n";
            std::cout << "[CodeGen] ASSIGN literal: " << instr.operand1 << " -> " << res_loc << "\n";
        }
        else if (op1_loc != res_loc)
        {
            // mem-to-mem needs a scratch; reg-reg / reg-mem / mem-reg are fine in one mov
            if (res_loc.find("[rbp") != std::string::npos && op1_loc.find("[rbp") != std::string::npos)
            {
                (*out_) << "    mov rax, " << op1_loc << "\n";
                (*out_) << "    mov " << res_loc << ", rax\n";
            }
            else
            {
                (*out_) << "    mov " << res_loc << ", " << op1_loc << "\n";
            }
            std::cout << "[CodeGen] ASSIGN: " << op1_loc << " -> " << res_loc << "\n";
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
        (*out_) << "    mov rbx, " << op2_loc << "\n";
        (*out_) << "    idiv rbx\n";
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
    case IROpcode::ADDR:
    {
        // operand1 is a stack variable name — take its address
        auto off = var_offsets_.find(instr.operand1);
        if (off == var_offsets_.end())
            throw std::runtime_error("ADDR: unknown variable '" + instr.operand1 + "'");
        (*out_) << "    lea rax, [rbp - " << off->second << "]\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    }
    case IROpcode::LOAD:
        (*out_) << "    mov rbx, " << op1_loc << "\n";
        (*out_) << "    mov rax, qword [rbx]\n";
        (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    case IROpcode::STORE:
        (*out_) << "    mov rbx, " << op1_loc << "\n";
        (*out_) << "    mov rax, " << op2_loc << "\n";
        (*out_) << "    mov qword [rbx], rax\n";
        break;
    case IROpcode::JUMP:
    {
        std::string target = instr.operand1;
        if (target.empty())
            target = infer_target_label_for_current_block();
        if (target.empty())
        {
            (*out_) << "    ; missing jump target in " << current_function_ << " " << current_block_label_ << "\n";
        }
        else
        {
            (*out_) << "    jmp " << target << "\n";
        }
        break;
    }
    case IROpcode::JUMPIF:
    {
        std::string target = instr.operand2;
        if (target.empty())
            target = infer_target_label_for_current_block();
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, 0\n";
        if (target.empty())
            (*out_) << "    ; missing jump target (JUMPIF)\n";
        else
            (*out_) << "    jne " << target << "\n";
        break;
    }
    case IROpcode::JUMPIFNOT:
    {
        std::string target = instr.operand2;
        if (target.empty())
            target = infer_target_label_for_current_block();
        (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    cmp rax, 0\n";
        if (target.empty())
            (*out_) << "    ; missing jump target (JUMPIFNOT)\n";
        else
            (*out_) << "    je " << target << "\n";
        break;
    }
    case IROpcode::RETURN:
        if (!instr.operand1.empty())
            (*out_) << "    mov rax, " << op1_loc << "\n";
        (*out_) << "    jmp " << current_function_ << "_epilogue\n";
        break;
    case IROpcode::CALL:
    {
        // Built-in print(int) → printf("%d\n", value) via libc
        if (instr.operand1 == "print")
        {
            if (instr.args.size() != 1)
                throw std::runtime_error("Built-in print expects exactly one argument");

            (*out_) << "    mov rdi, fmt_int\n";
            (*out_) << "    mov rsi, " << arg_locs[0] << "\n";
            (*out_) << "    xor eax, eax\n"; // 0 vector args (System V variadic ABI)
            (*out_) << "    call printf\n";

            if (!instr.result.empty())
                (*out_) << "    mov " << res_loc << ", rax\n";
            break;
        }

        static const char* param_regs[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
        const size_t reg_count = 6;
        const size_t n = instr.args.size();
        const size_t stack_args = (n > reg_count) ? (n - reg_count) : 0;
        const bool pad_stack = (stack_args % 2) != 0;

        if (pad_stack)
            (*out_) << "    sub rsp, 8\n";

        for (size_t i = 0; i < stack_args; ++i)
        {
            size_t arg_index = n - 1 - i;
            (*out_) << "    mov rax, " << arg_locs[arg_index] << "\n";
            (*out_) << "    push rax\n";
        }

        for (size_t i = 0; i < n && i < reg_count; ++i)
            (*out_) << "    mov " << param_regs[i] << ", " << arg_locs[i] << "\n";

        (*out_) << "    call " << instr.operand1 << "\n";

        if (stack_args > 0 || pad_stack)
            (*out_) << "    add rsp, " << ((stack_args + (pad_stack ? 1 : 0)) * 8) << "\n";

        if (!instr.result.empty())
            (*out_) << "    mov " << res_loc << ", rax\n";
        break;
    }
    default:
        throw std::runtime_error("Unsupported IR opcode in NASM codegen");
    }

    if (!instr.result.empty() && instr.opcode != IROpcode::JUMP && instr.opcode != IROpcode::JUMPIF
        && instr.opcode != IROpcode::JUMPIFNOT && instr.opcode != IROpcode::RETURN)
    {
        last_written_loc_ = res_loc;
    }

    // Return registers to the pool when a temporary is consumed for the last time
    switch (instr.opcode)
    {
    case IROpcode::JUMP:
        break;
    case IROpcode::JUMPIF:
    case IROpcode::JUMPIFNOT:
        consume_temp(instr.operand1);
        break;
    case IROpcode::CALL:
        for (const auto& arg : instr.args)
            consume_temp(arg);
        break;
    default:
        consume_temp(instr.operand1);
        consume_temp(instr.operand2);
        for (const auto& arg : instr.args)
            consume_temp(arg);
        break;
    }

    if (is_ir_temp(instr.result))
    {
        auto uit = temp_remaining_uses_.find(instr.result);
        if (uit != temp_remaining_uses_.end() && uit->second == 0)
            consume_temp(instr.result);
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
    if (known_functions_.count(name))
        return name;

    if (is_ir_temp(name))
        return allocate_temp_location(name);

    auto it = var_offsets_.find(name);
    if (it != var_offsets_.end())
        return "[rbp - " + std::to_string(it->second) + "]";

    int newOff = stack_offset_ + 8;
    stack_offset_ = newOff;
    var_offsets_[name] = newOff;
    std::cout << "[CodeGen] get_loc: allocated new var '" << name << "' offset=" << newOff << "\n";
    return "[rbp - " + std::to_string(newOff) + "]";
}

std::string CodeGenerator::find_label_with_substr(const std::string& substr) const
{
    auto it = std::find_if(block_labels_.begin(), block_labels_.end(),
        [&substr](const std::string& lbl)
        {
            return lbl.find(substr) != std::string::npos;
        });
    return it != block_labels_.end() ? *it : "";
}

std::string CodeGenerator::infer_target_label_for_current_block() const
{
    if (current_block_label_.find("body") != std::string::npos)
    {
        std::string found = find_label_with_substr("cond");
        if (!found.empty())
            return found;
    }
    size_t bidx = block_index_.at(current_block_label_);
    if (bidx + 1 < block_labels_.size())
        return block_labels_[bidx + 1];
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
            if (!instr.result.empty() && labels_.count(instr.result) == 0 && known_functions_.count(instr.result) == 0)
                all_vars.insert(instr.result);

            auto consider = [&](const std::string& name)
            {
                if (name.empty())
                    return;
                if (name.find_first_not_of("0123456789") == std::string::npos)
                    return;
                if (labels_.count(name) || known_functions_.count(name))
                    return;
                all_vars.insert(name);
            };

            if (instr.opcode != IROpcode::CALL)
                consider(instr.operand1);
            consider(instr.operand2);
            for (const auto& arg : instr.args)
                consider(arg);
        }
    }

    // Skip IR temps that live entirely in registers
    for (auto it = all_vars.begin(); it != all_vars.end();)
    {
        if (is_ir_temp(*it) && temp_to_reg_.count(*it) && !spilled_temps_.count(*it))
            it = all_vars.erase(it);
        else
            ++it;
    }

    std::vector<std::string> params;
    std::transform(func.parameters.begin(), func.parameters.end(),
        std::back_inserter(params),
        [](const auto& p)
        { return p.name; });

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
        offset += 8;
        var_offsets_[p] = offset;
        std::cout << "Param: " << p << " Offset: " << offset << "\n";
    }
    for (const auto& v : locals)
    {
        offset += 8;
        var_offsets_[v] = offset;
        std::cout << "Local: " << v << " Offset: " << offset << "\n";
    }

    stack_offset_ = offset;
    if (stack_offset_ % 16 != 0)
        stack_offset_ = ((stack_offset_ + 15) / 16) * 16;

    std::cout << "[CodeGen] allocate_stack done: final_stack_offset=" << stack_offset_
              << " var_count=" << var_offsets_.size() << "\n";
}

} // namespace minic
