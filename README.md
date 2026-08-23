# miniC

miniC is a lightweight compiler for a simplified C-like language. It processes source code through these stages:

1. **Lexing** — tokenization of input source  
2. **Parsing** — Abstract Syntax Tree (AST)  
3. **Semantic analysis** — types, scopes, control-flow checks  
4. **IR generation** — flat intermediate representation  
5. **Code generation** — NASM x86-64 assembly (System V ABI, libc)

**Language highlights:** `int` / `int *`, functions & recursion, `if` / `while` / `break` / `continue`, builtin `print(int)`.

**Documentation:** start at [docs/README.md](./docs/README.md) — full [Language Specification](./docs/LanguageSpecification.md) and [Compiler Architecture](./docs/CompilerArchitecture.md).

---

## Project Structure

- [CMakeLists.txt](./CMakeLists.txt) — top-level CMake configuration  
- [docs/](./docs/README.md) — language, architecture, development guides  
- [include/minic/](./include/minic/) — public headers  
- [src/](./src/) — compiler implementation  
- [tests/](./tests/) — Google Test unit suite  
- [e2e_tests/](./e2e_tests/) — assemble / link / stdout comparison  

---

## Example

```c
int main() {
    int x = 5;
    int *p = &x;
    while (*p < 10) {
        *p = *p + 1;
        if (*p == 8) {
            break;
        }
    }
    print(x);
    return 0;
}
```

```bash
./build/src/minic program.mc   # writes ./output.asm
nasm -f elf64 output.asm -o output.o
gcc -no-pie output.o -o output
./output
```

Pipeline, stack frames, and register allocation are documented in [CompilerArchitecture.md](./docs/CompilerArchitecture.md).

---

## Assembling `output.asm` into an executable

miniC emits NASM x86-64 assembly that links against **libc** (entry point is C `main`, built-in `print` calls `printf`).

```bash
# Assemble object file (ELF64)
nasm -f elf64 output.asm -o output.o

# Link with GCC/libc (-no-pie avoids PIE relocation issues with NASM)
gcc -no-pie output.o -o output

# Run
./output
```

One-liner:

```bash
nasm -f elf64 output.asm -o output.o && gcc -no-pie output.o -o output && ./output
```

Built-in `print(int)` example:

```c
int main() {
    print(42);
    return 0;
}
```

Requirements: `nasm` and `gcc` on a Linux x86-64 system.

---

## Building the project

```bash
# Developer mode (debug + ASan + UBSan)
cmake -S . -B build -DPRODUCTION=OFF -DBUILD_TESTS=ON
cmake --build build -j

# Production mode
cmake -S . -B build -DPRODUCTION=ON -DBUILD_TESTS=ON
cmake --build build -j

# Run tests
ctest --test-dir build --output-on-failure
```

### API documentation (Doxygen)

With Doxygen installed, HTML docs are generated from comments in `include/minic/`:

```bash
cmake -S . -B build -DBUILD_DOCS=ON
cmake --build build --target docs
# Open: build/docs/html/index.html
```

### End-to-end tests

See [e2e_tests/README.md](./e2e_tests/README.md). Quick start:

```bash
python3 e2e_tests/run_e2e.py          # Linux with nasm+gcc
python3 e2e_tests/run_e2e.py --docker # isolated Ubuntu toolchain
```
