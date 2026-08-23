# miniC End-to-End tests

These tests compile `.mc` sources with **minic**, assemble with **nasm**, link with **gcc** (libc / `printf`), run the binary, and compare **stdout** to `*.expected`.

## Cases

| File | Covers |
|------|--------|
| `01_break_continue.mc` | `while` + `break` / `continue` + `print` |
| `02_pointers.mc` | `&`, `*`, stores through pointers |
| `03_factorial.mc` | recursion (`fact`) |
| `04_calls_print.mc` | nested calls + `print` |
| `05_nested_combo.mc` | nested control flow + pointers + `print` |

## Prerequisites

- Built `minic` binary (CMake target `minic`)
- On Linux host: `nasm`, `gcc`, `python3`
- Or Docker (works on Windows/macOS too)

## Run locally (Linux)

```bash
cmake -S . -B build -DPRODUCTION=ON -DBUILD_TESTS=OFF -DBUILD_DOCS=OFF
cmake --build build -j

python3 e2e_tests/run_e2e.py
# or: bash e2e_tests/run_e2e.sh
```

Custom compiler path:

```bash
python3 e2e_tests/run_e2e.py --compiler build/src/minic
```

## Run in Docker

```bash
python3 e2e_tests/run_e2e.py --docker
```

This builds image `minic-e2e`, compiles miniC inside the container, then runs all E2E cases.

## Filter

```bash
python3 e2e_tests/run_e2e.py --filter factorial
```
