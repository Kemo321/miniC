# miniC Development Guide

**Audience:** Contributors and maintainers  
**Companion docs:** [Language Specification](./LanguageSpecification.md) · [Compiler Architecture](./CompilerArchitecture.md)

This guide covers building the toolchain, running tests, formatting code, and generating API documentation.

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Building](#2-building)
3. [Running the Compiler](#3-running-the-compiler)
4. [Assembling and Linking](#4-assembling-and-linking)
5. [Unit Tests](#5-unit-tests)
6. [End-to-End Tests](#6-end-to-end-tests)
7. [Code Formatting](#7-code-formatting)
8. [API Documentation (Doxygen)](#8-api-documentation-doxygen)
9. [CMake Options](#9-cmake-options)
10. [Contribution Checklist](#10-contribution-checklist)

---

## 1. Prerequisites

| Tool | Purpose |
|------|---------|
| C++23 compiler | GCC, Clang, or MSVC |
| CMake ≥ 3.20 | Build system |
| Ninja or Make / MSBuild | Build backend |
| Google Test | Fetched via CMake when `BUILD_TESTS=ON` |
| `nasm` + `gcc` (Linux) | Assemble/link generated asm |
| Docker (optional) | E2E on Windows/macOS |
| Doxygen (optional) | HTML API docs |
| `clang-format` (optional) | Style enforcement |

---

## 2. Building

### 2.1 Developer configuration

Debug symbols and sanitizers (ASan / UBSan) on GCC/Clang when `PRODUCTION=OFF`:

```bash
cmake -S . -B build -DPRODUCTION=OFF -DBUILD_TESTS=ON
cmake --build build -j
```

> **Note:** On MSVC, sanitizers are not enabled the same way; the build uses `/W4` instead.

### 2.2 Production configuration

```bash
cmake -S . -B build -DPRODUCTION=ON -DBUILD_TESTS=ON
cmake --build build -j
```

### 2.3 Artifact location

The compiler binary is typically:

```text
build/src/minic          # Unix
build/src/Debug/minic.exe  # MSVC multi-config (path may vary)
```

---

## 3. Running the Compiler

```bash
./build/src/minic path/to/program.mc
```

Successful compilation writes **`output.asm`** in the **current working directory**.

```bash
cd /tmp
/path/to/minic ~/miniC/e2e_tests/03_factorial.mc
# → /tmp/output.asm
```

---

## 4. Assembling and Linking

Target: **Linux x86-64**, libc, non-PIE:

```bash
nasm -f elf64 output.asm -o output.o
gcc -no-pie output.o -o output
./output
```

One-liner:

```bash
nasm -f elf64 output.asm -o output.o && gcc -no-pie output.o -o output && ./output
```

| Symbol | Role |
|--------|------|
| `main` | Program entry (`global main`) |
| `printf` | Used by builtin `print` (`extern printf`) |

---

## 5. Unit Tests

```bash
ctest --test-dir build --output-on-failure
# or run the test binary directly, e.g.:
./build/tests/minic_tests
```

Coverage areas (Google Test):

| Suite | Focus |
|-------|--------|
| Lexer | Tokens, comments, strings |
| Parser | Grammar constructs, AST shape |
| Semantic | Types, scopes, `break`/`continue`, pointers |
| IR | Lowering, calls, loops |
| AST | Node structure / visitors |

---

## 6. End-to-End Tests

See also [e2e_tests/README.md](../e2e_tests/README.md).

```bash
# Host Linux with nasm + gcc
python3 e2e_tests/run_e2e.py --compiler build/src/minic

# Docker (recommended on Windows)
python3 e2e_tests/run_e2e.py --docker
```

Cases exercise `print`, recursion, pointers, and `break`/`continue`.

---

## 7. Code Formatting

From the repository root (Unix-like shells):

```bash
clang-format -i -style=file $(find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.c" -o -name "*.hpp" \) ! -path "./build*" ! -path "./build-tests*")
```

On Windows PowerShell, format known trees explicitly, for example:

```powershell
Get-ChildItem -Recurse include,src,tests -Include *.cpp,*.hpp,*.h |
  ForEach-Object { clang-format -i -style=file $_.FullName }
```

---

## 8. API Documentation (Doxygen)

```bash
cmake -S . -B build -DBUILD_DOCS=ON
cmake --build build --target docs
```

Open `build/docs/html/index.html`. This is **API reference** generated from headers; narrative docs live in this `docs/` folder.

---

## 9. CMake Options

| Option | Default | Meaning |
|--------|---------|---------|
| `PRODUCTION` | OFF | Disable sanitizers / enable release-oriented flags |
| `BUILD_TESTS` | — | Build Google Test suite |
| `BUILD_DOCS` | OFF | Enable Doxygen `docs` target |

> **Tip:** Prefer an out-of-tree build directory (`build/`, `build-tests/`) and never commit object files.

---

## 10. Contribution Checklist

1. Match existing style in `include/minic/` and `src/`.
2. Add or extend unit tests for frontend/IR changes.
3. Add an E2E `.mc` + `.expected` pair for user-visible behavior.
4. Update [LanguageSpecification.md](./LanguageSpecification.md) if syntax or semantics change.
5. Update [CompilerArchitecture.md](./CompilerArchitecture.md) if the pipeline or ABI contract changes.
6. Run unit tests and E2E (Docker if not on Linux).

---

## See also

- [Documentation Home](./README.md)
- [Language Specification](./LanguageSpecification.md)
- [Compiler Architecture](./CompilerArchitecture.md)
