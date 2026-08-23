# miniC Language Specification

**Version:** 1.0  
**Status:** Stable (educational / production-quality toolchain)  
**Target:** Linux x86-64 (System V AMD64 ABI, NASM + libc)

This document is the normative description of the **miniC** programming language as implemented by the miniC compiler. It defines lexical structure, types, operators, statements, scoping rules, and the built-in runtime API.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Lexical Structure](#2-lexical-structure)
3. [Types](#3-types)
4. [Declarations](#4-declarations)
5. [Expressions and Operators](#5-expressions-and-operators)
6. [Statements and Control Flow](#6-statements-and-control-flow)
7. [Functions](#7-functions)
8. [Pointers](#8-pointers)
9. [Name Binding and Scope](#9-name-binding-and-scope)
10. [Built-in API](#10-built-in-api)
11. [Program Structure and Semantics](#11-program-structure-and-semantics)
12. [Complete Examples](#12-complete-examples)
13. [Appendix: Grammar Sketch](#13-appendix-grammar-sketch)

---

## 1. Introduction

### 1.1 Design goals

miniC is a **C-like** language intentionally reduced to a clear, teachable core while still exercising a full compiler pipeline:

| Goal | How miniC addresses it |
|------|------------------------|
| Familiar syntax | C-style declarations, braces, operators |
| Explicit control flow | `if` / `else`, `while`, `break`, `continue` |
| Procedures | Top-level functions, recursion, call-by-value |
| Memory awareness | First-class `int *` pointers (`&` / `*`) |
| Observable I/O | Built-in `print(int)` lowered to `printf` |

### 1.2 What miniC is not

> **Note:** miniC is **not** a complete C dialect. There is no preprocessor, no floating-point types, no arrays, no structs, no `for`/`switch`, no `goto`, and no user-defined types beyond `int *`.

### 1.3 Compilation model (summary)

Source files (conventionally `*.mc` or `*.cmm`) are compiled to NASM assembly that links against **libc**. The program entry point is the C-compatible function `main`.

```text
source.mc  →  Lexer → Parser → Semantic Analyzer → IR → NASM  →  executable
```

For pipeline details, see [CompilerArchitecture.md](./CompilerArchitecture.md).

---

## 2. Lexical Structure

### 2.1 Character set and whitespace

Source text is processed as a stream of characters. Spaces, tabs, and carriage returns are whitespace. **Newlines** are significant to the lexer as `NEWLINE` tokens (used for line tracking and recovery), but they do not terminate statements—statements end with `;`.

### 2.2 Comments

| Form | Syntax | Notes |
|------|--------|-------|
| Line comment | `// …` | Runs to end of line |
| Block comment | `/* … */` | May span lines; non-nesting |

```c
// Compute next Fibonacci number
int next = a + b; /* temporary */
```

### 2.3 Keywords

The following identifiers are reserved:

| Keyword | Role |
|---------|------|
| `int` | Integer type / return type |
| `void` | Empty return type (functions only) |
| `string` | String type |
| `if` | Conditional |
| `else` | Alternate branch |
| `while` | Loop |
| `return` | Function return |
| `break` | Exit nearest loop |
| `continue` | Next loop iteration |

### 2.4 Identifiers

```text
identifier := letter | '_' | '$' , { letter | digit | '_' | '$' }
```

Identifiers are case-sensitive. Examples: `main`, `_tmp`, `x$1`.

### 2.5 Literals

| Kind | Form | Type |
|------|------|------|
| Integer | Decimal digits (`42`, `0`) | `int` |
| String | `"…"` with escapes `\n`, `\t`, `\\`, `\"` | `string` |

> **Warning:** Unclosed string literals and unknown escape sequences are **hard errors** at lex time.

### 2.6 Operators and punctuation

| Tokens | Meaning |
|--------|---------|
| `+` `-` `*` `/` | Arithmetic (`*` also unary dereference) |
| `=` | Assignment |
| `==` `!=` `<` `>` `<=` `>=` | Comparison |
| `!` | Logical not (truthiness on `int`) |
| `&` | Address-of |
| `( )` `{ }` `,` `;` | Grouping, blocks, lists, statement end |

---

## 3. Types

### 3.1 Built-in types

| Type | Description | Size (codegen) |
|------|-------------|----------------|
| `int` | Signed integer | 8 bytes (qwords on stack) |
| `string` | String value | Supported in frontend; limited codegen |
| `void` | No value | Only as function return type |
| `int *` | Pointer to `int` | 8-byte address |

Internally, the semantic analyzer represents `int *` as a distinct type (`TYPE_PTR_INT`), never interchangeable with `int`.

### 3.2 Type compatibility

| Context | Rule |
|---------|------|
| Assignment / initialization | Exact type match required |
| Arithmetic / comparison | Both operands must be `int` |
| Conditions (`if` / `while`) | Condition must be `int` (nonzero = true) |
| `&` operand | Must be an `int` **variable** |
| `*` operand | Must be `int *` |
| Function arguments | Must match declared parameter types |

```c
int x = 1;
int *p = &x;   // OK: int* ← address of int
// p = x;      // ERROR: int* ← int
// int y = p;  // ERROR: int ← int*
```

---

## 4. Declarations

### 4.1 Variable declarations

```c
type declarator ;
type declarator = expression ;
```

`type` is `int`, `string`, or `int` followed by `*` for pointers:

```c
int x;
int y = 10;
int *p = &y;
string msg = "hi";
```

> **Warning:** Variables cannot be declared `void`. Pointer types other than `int *` are rejected.

### 4.2 Function declarations

All functions are **top-level**. There are no nested functions and no separate prototypes—definition is declaration.

```c
return_type name ( parameter-list ) block
```

```c
int add(int a, int b) {
    return a + b;
}

void greet(int n) {
    print(n);
}
```

Parameters may be `int`, `string`, or `int *`:

```c
int load(int *p) {
    return *p;
}
```

---

## 5. Expressions and Operators

### 5.1 Precedence (high → low)

| Level | Operators | Associativity |
|-------|-----------|---------------|
| 1 (highest) | Primary: literals, identifiers, calls, `(expr)` | — |
| 2 | Unary: `!` `-` `*` `&` | Right |
| 3 | `*` `/` (binary) | Left |
| 4 | `+` `-` | Left |
| 5 (lowest) | `==` `!=` `<` `>` `<=` `>=` | Left |

### 5.2 Arithmetic

```c
int a = 2 + 3 * 4;   // 14
int b = (2 + 3) * 4; // 20
int c = -a;
```

Division truncates toward zero (integer `idiv` semantics).

### 5.3 Comparison and boolean interpretation

Comparisons yield `int` (`1` or `0`). Unary `!` treats `0` as false and anything else as true.

```c
int ok = x > 0;
int none = !ok;
```

### 5.4 Function calls

```c
callee ( argument-list )
```

Arguments are evaluated left-to-right and passed by value (pointers pass the address value).

```c
int r = add(mul(2, 3), 4);
print(r);
```

### 5.5 Assignment expressions vs statements

Assignment is a **statement** form `name = expr;` (or `*ptr = expr;`), not an expression. Chained assignment like `a = b = 1` is not supported.

---

## 6. Statements and Control Flow

### 6.1 Blocks

```c
{
    statement*
}
```

Blocks introduce a **new scope** for local declarations (see §9).

### 6.2 Expression statements

Any expression followed by `;` is valid, including discarded calls:

```c
print(42);
foo();
```

### 6.3 `if` / `else`

```c
if ( condition ) block
if ( condition ) block else block
```

`condition` must have type `int`.

```c
if (x > 0) {
    print(1);
} else {
    print(0);
}
```

### 6.4 `while`

```c
while ( condition ) block
```

```c
while (i < 10) {
    i = i + 1;
}
```

### 6.5 `break` and `continue`

| Statement | Meaning |
|-----------|---------|
| `break;` | Jump out of the **nearest enclosing** `while` |
| `continue;` | Jump to the condition of the **nearest enclosing** `while` |

> **Error:** `break` or `continue` outside any loop is a **semantic error**.

```c
while (i < 10) {
    i = i + 1;
    if (i == 3) {
        continue;
    }
    if (i == 8) {
        break;
    }
    print(i);
}
```

### 6.6 `return`

```c
return ;
return expression ;
```

- Non-`void` functions must return a value of the declared type.
- `void` functions must use bare `return;` (or fall through—codegen still emits an epilogue).

---

## 7. Functions

### 7.1 Calling convention (runtime)

Generated code follows the **System V AMD64 ABI**:

- Integer/pointer arguments: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`, then stack
- Return value: `rax`
- Callee uses a standard frame (`push rbp` / `mov rbp, rsp`)

### 7.2 Recursion

Direct and mutual recursion are allowed. Example: factorial.

```c
int fact(int n) {
    if (n < 2) {
        return 1;
    }
    return n * fact(n - 1);
}
```

### 7.3 Entry point

A program should define:

```c
int main() { … }
```

The assembler output declares `global main` and links with libc (no freestanding `_start`).

---

## 8. Pointers

### 8.1 Forming addresses

```c
& variable
```

The operand must be an **`int` variable** (lvalue). Taking the address of an expression, literal, or `string` is illegal.

### 8.2 Dereference

```c
* pointer-expression
```

Yields an `int` lvalue/rvalue depending on context.

### 8.3 Store through pointer

```c
* pointer-expression = expression ;
```

```c
int x = 10;
int *p = &x;
*p = 20;     // x is now 20
print(*p);
```

### 8.4 Pointer parameters

```c
void bump(int *p) {
    *p = *p + 1;
}

int main() {
    int x = 41;
    bump(&x);
    print(x);  // 42
    return 0;
}
```

---

## 9. Name Binding and Scope

### 9.1 Scope levels

| Level | Contents |
|-------|----------|
| Global | Function names (and built-ins such as `print`) |
| Function | Parameters + body locals |
| Block | Declarations inside `{ }` of `if` / `while` / nested blocks |

### 9.2 Rules

1. **Redeclaration** of a name in the **same** scope is an error.
2. Inner scopes may **shadow** outer names.
3. Use of an undeclared identifier is an error.
4. Function names occupy a separate global table; redefining `print` is rejected.

```c
int main() {
    int x = 1;
    if (1) {
        int x = 2;  // OK: shadows outer x
        print(x);   // 2
    }
    print(x);       // 1
    return 0;
}
```

---

## 10. Built-in API

### 10.1 `print`

| Item | Definition |
|------|------------|
| Signature | `void print(int value)` |
| Effect | Writes decimal representation followed by newline |
| Implementation | Lowered to `printf("%d\n", value)` |

```c
int main() {
    print(42);
    return 0;
}
```

Typical output:

```text
42
```

> **Note:** There is no built-in `print` overload for `string` in the current release. Passing a non-`int` argument is a type error.

---

## 11. Program Structure and Semantics

### 11.1 Translation unit

A miniC file is a sequence of function definitions. There are no global variables.

### 11.2 Evaluation order

- Binary operands: left, then right.
- Call arguments: left-to-right.
- Side effects of `print` and stores through pointers become visible in that order.

### 11.3 Undefined / implementation-defined behavior

| Situation | Handling |
|-----------|----------|
| Division by zero | Runtime CPU exception (not diagnosed) |
| Null / invalid pointer dereference | Undefined (same as unsafe C) |
| Missing `return` in non-void path | May fall into epilogue with garbage `rax` |

---

## 12. Complete Examples

### 12.1 Control flow and I/O

```c
int main() {
    int i = 0;
    while (i < 10) {
        i = i + 1;
        if (i == 3) {
            continue;
        }
        if (i == 8) {
            break;
        }
        print(i);
    }
    return 0;
}
```

### 12.2 Recursion

```c
int fact(int n) {
    if (n < 2) {
        return 1;
    }
    return n * fact(n - 1);
}

int main() {
    print(fact(5));
    return 0;
}
```

### 12.3 Pointers

```c
int main() {
    int x = 10;
    int *p = &x;
    *p = *p + 5;
    print(x);
    return 0;
}
```

---

## 13. Appendix: Grammar Sketch

Informal EBNF (simplified):

```ebnf
program     = { function } ;
function    = type ident "(" [ params ] ")" block ;
params      = param { "," param } ;
param       = type ident ;
type        = "int" [ "*" ] | "void" | "string" ;

block       = "{" { statement } "}" ;
statement   = var_decl | assign | deref_assign | expr_stmt
            | if_stmt | while_stmt | return_stmt
            | break_stmt | continue_stmt ;

var_decl    = type ident [ "=" expr ] ";" ;
assign      = ident "=" expr ";" ;
deref_assign= "*" unary "=" expr ";" ;
expr_stmt   = expr ";" ;
if_stmt     = "if" "(" expr ")" block [ "else" block ] ;
while_stmt  = "while" "(" expr ")" block ;
return_stmt = "return" [ expr ] ";" ;
break_stmt  = "break" ";" ;
continue_stmt = "continue" ";" ;

expr        = comparison ;
comparison  = term { relop term } ;
term        = factor { ("+" | "-") factor } ;
factor      = unary { ("*" | "/") unary } ;
unary       = ( "!" | "-" | "*" | "&" ) unary | primary ;
primary     = literal | ident [ "(" [ args ] ")" ] | "(" expr ")" ;
```

---

## See also

- [Compiler Architecture](./CompilerArchitecture.md) — pipeline, IR, ABI, register allocation  
- [Development Guide](./DevelopmentGuide.md) — build, unit tests, E2E  
- [Documentation Home](./README.md)
