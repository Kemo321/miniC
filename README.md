````markdown
# miniC

miniC is a lightweight compiler for a simplified C-like language. It processes source code through these stages:

1. **Lexing**: Tokenization of input source.  
2. **Parsing**: Building an Abstract Syntax Tree (AST).  
3. **Semantic Analysis**: Type checking and error detection.  
4. **IR Generation**: Intermediate Representation (IR) for optimization.  
5. **Code Generation**: Outputting executable code (NASM assembly).

Key features:
- Modular design with separate components for each stage.
- Built with CMake for cross-platform builds.
- Includes unit tests using Google Test.
- Supports basic C syntax (variables, functions, control flow).

---

## Project Structure
- [CMakeLists.txt](./CMakeLists.txt) — Top-level CMake configuration  
- docs/
    - [dev.md](./docs/dev.md)
    - [ASTVisitor.md](./docs/ASTVisitor.md)
    - [CodeGenerator.md](./docs/CodeGenerator.md)
    - [IRGenerator.md](./docs/IRGenerator.md)
    - [IR.md](./docs/IR.md)
    - [Lexer.md](./docs/Lexer.md)
    - [Parser.md](./docs/Parser.md)
    - [SemanticAnalyzer.md](./docs/SemanticAnalyzer.md)
    - [Token.md](./docs/Token.md)
- include/
    - minic/
        - [AST.hpp](./include/minic/AST.hpp)
        - [ASTVisitor.hpp](./include/minic/ASTVisitor.hpp)
        - [CodeGenerator.hpp](./include/minic/CodeGenerator.hpp)
        - [IRGenerator.hpp](./include/minic/IRGenerator.hpp)
        - [IR.hpp](./include/minic/IR.hpp)
        - [Lexer.hpp](./include/minic/Lexer.hpp)
        - [Parser.hpp](./include/minic/Parser.hpp)
        - [SemanticAnalyzer.hpp](./include/minic/SemanticAnalyzer.hpp)
        - [Token.hpp](./include/minic/Token.hpp)
- [README.md](./README.md) — Root README  
- src/
    - [CMakeLists.txt](./src/CMakeLists.txt)
    - [CodeGenerator.cpp](./src/CodeGenerator.cpp)
    - [IRGenerator.cpp](./src/IRGenerator.cpp)
    - [Lexer.cpp](./src/Lexer.cpp)
    - [main.cpp](./src/main.cpp)
    - [Parser.cpp](./src/Parser.cpp)
    - [SemanticAnalyzer.cpp](./src/SemanticAnalyzer.cpp)
- tests/
    - [CMakeLists.txt](./tests/CMakeLists.txt)
    - [main.cpp](./tests/main.cpp)
    - [TestAST.cpp](./tests/TestAST.cpp)
    - [TestExample.cpp](./tests/TestExample.cpp)
    - [TestIRGenerator.cpp](./tests/TestIRGenerator.cpp)
    - [TestLexer.cpp](./tests/TestLexer.cpp)
    - [TestParser.cpp](./tests/TestParser.cpp)
    - [TestSemanticAnalyzer.cpp](./tests/TestSemanticAnalyzer.cpp)

---

## Example Compilation

### Input (miniC source)

```c
int main() {
    int x = 5;
    if (x > 0) {
        while (x < 10) {
            x = x + 1;
        }
    }
    return x;
}
````

### Output (NASM x86-64 assembly)

```asm
section .data
section .text
global _start
_start:
    call main
    mov rdi, rax
    mov rax, 60
    syscall

main:
    push rbp
    mov rbp, rsp
    sub rsp, 64
entry_0:
    mov qword [rbp - 8], 5
    mov rax, [rbp - 8]
    mov [rbp - 64], rax
    mov qword [rbp - 16], 0
    mov rax, [rbp - 64]
    cmp rax, [rbp - 16]
    setg al
    movzx rax, al
    mov [rbp - 24], rax
    mov rax, [rbp - 24]
    cmp rax, 0
    je if_else_2
if_then_1:
    jmp while_cond_4
while_cond_4:
    mov qword [rbp - 32], 10
    mov rax, [rbp - 64]
    cmp rax, [rbp - 32]
    setl al
    movzx rax, al
    mov [rbp - 40], rax
    mov rax, [rbp - 40]
    cmp rax, 0
    je while_end_6
while_body_5:
    mov qword [rbp - 48], 1
    mov rax, [rbp - 64]
    add rax, [rbp - 48]
    mov [rbp - 56], rax
    mov rax, [rbp - 56]
    mov [rbp - 64], rax
    jmp while_cond_4
while_end_6:
    jmp if_end_3
if_else_2:
    jmp if_end_3
if_end_3:
    mov rax, [rbp - 64]
    jmp main_epilogue
main_epilogue:
    leave
    ret
```

---

## Explanation of the NASM Code

1. **Program entry**

   * `_start` is the Linux entry point (instead of relying on libc).
   * It calls `main`, retrieves the return value in `rax`, and invokes the `exit` system call with that value.

2. **Stack frame setup**

   * `push rbp` / `mov rbp, rsp` establish a base pointer.
   * `sub rsp, 64` reserves 64 bytes of stack space for local variables and temporaries.

3. **Variable initialization**

   * `int x = 5;` becomes `mov qword [rbp - 8], 5` and then copied into `[rbp - 64]`.
   * miniC uses stack slots for both declared variables and intermediate results.

4. **If condition (`x > 0`)**

   * `cmp` compares values.
   * `setg al` sets a flag if greater.
   * The result is stored in another stack slot (`[rbp - 24]`) and checked.

5. **While loop (`while (x < 10)`)**

   * Loop condition: compare `x` with `10`.
   * If true, execution continues into the loop body; otherwise, it jumps to `while_end_6`.
   * The loop body increments `x` by 1.

6. **Return value**

   * At the end, the function moves the final value of `x` into `rax`, the return register.
   * `_start` uses this to exit the program with the correct return code.

---

## Why is Memory Allocated for Every Statement?

You may notice that **every intermediate computation is written back to the stack** (`rbp - 16`, `rbp - 24`, `rbp - 32`, etc.) instead of keeping values purely in registers.

This happens because:

* **Naive code generation**: miniC currently generates straightforward stack-based code. Each sub-expression or condition is lowered into a temporary slot on the stack. This avoids the complexity of **register allocation**.
* **Simplicity over optimization**: By storing results explicitly in memory, the compiler ensures correctness without worrying about limited register availability or lifetime analysis.
* **Debugging clarity**: Using stack slots makes it easy to map AST nodes and IR instructions directly to memory locations, which is useful during development.

In more advanced compilers, an **optimization pass** or a **register allocator** would keep many of these values in CPU registers, drastically reducing memory usage and improving performance. For now, miniC favors **correctness and simplicity** over efficiency.

---