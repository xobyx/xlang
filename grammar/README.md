# xlang Formal PEG Grammar Specification

This directory contains the formal **Parsing Expression Grammar (PEG)** specification for **xlang** (`.xb`), located in [`xlang.peg`](file:///home/xobyx/xlang/grammar/xlang.peg).

---

## 1. Overview

Parsing Expression Grammars (Bryan Ford, 2004) describe a formal, unambiguous language syntax using ordered choice (`/`), syntactic predicates (`&` and `!`), and deterministic greedy repetition (`*`, `+`, `?`). Unlike traditional Context-Free Grammars (EBNF/BNF), PEGs never produce ambiguous parse trees.

The xlang PEG grammar matches the modern AST parser in [`xast_parser.c`](file:///home/xobyx/xlang/xast_parser.c) and lexical scanner in [`lexer.c`](file:///home/xobyx/xlang/lexer.c).

---

## 2. Key Language Grammar Characteristics

### 2.1 Lexical & Disambiguation Rules
- **Scannerless Design**: Whitespace `_` and comments (`#`, `//`, `/* ... */`) are handled uniformly.
- **Keyword Boundary Guards**: All keywords use negative lookahead `!IdentRest` (e.g. `"for" !IdentRest`) so identifiers like `format` or `foreign` are parsed as identifiers, not keywords.
- **Statement Separation (`StatementTerminator`)**: Statements are delimited by `;`, `\n`, `\r\n`, or enclosed within blocks `{ ... }`.
- **Lookahead Line Terminators (`LineTerminator`)**: Statements like `return` distinguish between a bare `return` and `return <expr>` using lookahead `!LineTerminator` without erroneously consuming expressions on subsequent lines.

### 2.2 Operator Precedence Hierarchy (14 Levels)

The grammar encodes operator precedence without left recursion through hierarchical sub-rules:

| Precedence Level | Rule Name | Operators | Associativity | Description |
| :---: | :--- | :--- | :---: | :--- |
| **1 (Lowest)** | `AssignmentExpr` | `=`, `+=`, `-=`, `*=`, `/=`, `%=` | Right | Variable and field assignment |
| **2** | `ExprOr` | `\|\|` | Left | Logical OR |
| **3** | `ExprAnd` | `&&` | Left | Logical AND |
| **4** | `ExprBitOr` | `\|` | Left | Bitwise OR |
| **5** | `ExprBitXor` | `^` | Left | Bitwise XOR |
| **6** | `ExprBitAnd` | `&` | Left | Bitwise AND |
| **7** | `ExprEquality` | `==`, `!=` | Left | Equality and inequality |
| **8** | `ExprRelational` | `<`, `<=`, `>`, `>=` | Left | Relational comparisons |
| **9** | `ExprShift` | `<<`, `>>` | Left | Bitwise shifts |
| **10** | `ExprAdditive` | `+`, `-` | Left | Addition, subtraction, string concat |
| **11** | `ExprMultiplicative` | `*`, `/`, `%` | Left | Multiplication, division, modulo |
| **12** | `ExprUnary` | `-`, `!`, `~` | Right (Prefix) | Negation, logical NOT, bitwise NOT |
| **13** | `ExprPostfix` | `.field`, `.method()`, `[idx]`, `++`, `--` | Left | Postfix chaining, indexing, inc/dec |
| **14 (Highest)** | `ExprPrimary` | `new`, calls, literals, `this`, identifiers, `(...)` | - | Primary expressions and grouping |

---

## 3. Grammar Syntax Breakdown

### 3.1 Statements
```peg
Statement <-
    ImportStmt
  / ClassDecl
  / FunctionDecl
  / VarDecl
  / IfStmt
  / WhileStmt
  / DoWhileStmt
  / ForStmt
  / ReturnStmt
  / BreakStmt
  / ContinueStmt
  / Block
  / ExprStmt
```

### 3.2 Loops
xlang supports modern and classic loop statements:
1. **3-Part For Loop (Standard & Modern)**:
   ```xlang
   for (int i = 0, i++, i < 5) {
       print(i)
   }
   ```
   Also supports compound steps (`i += 2`, `i = i + 1`), existing variables (`for (i = 0, i++, i < 5)`), and semicolon syntax (`for (int i = 0; i < 5; i++)`).
2. **For-in Loop**:
   ```xlang
   for (item in collection) { ... }
   for item in collection { ... }
   ```
3. **While & Do-While**:
   ```xlang
   while (condition) { ... }
   do { ... } while (condition)
   ```
4. **Classic Stepper Loop (Backward Compatible)**:
   ```xlang
   for i (0, i + 1, i < 5) { ... }
   ```

### 3.3 Object-Oriented Constructs
```peg
# Class declaration with optional base class
ClassDecl <- "class" __ Identifier ( _ "(" _ Identifier? _ ")" )? _ "{" _ ( ClassMember ( _ StatementTerminator )* )* _ "}"

# Chained instantiation and method calling
NewExpr <- "new" __ Identifier ( _ "(" _ ArgumentList? _ ")" )?
```
Supports direct chained method calls:
```xlang
new Calculator(10).add(5).result()
```

---

## 4. Verification & Testing

The PEG grammar has been validated against all 52 tests in `tests/*.xb`, including:
- Complex nested control flow (`tests/test_deep_nesting.xb`, `tests/test_while.xb`, `tests/test_for.xb`)
- Class inheritance and constructors (`tests/test_classes.xb`, `tests/test_constructor.xb`, `tests/test_inheritance.xb`)
- Chained calls and object instantiation (`tests/test_new_object.xb`)
- Dynamic collections and for-in iteration (`tests/test_collections_for_in.xb`)
- C extensions and Raylib graphics bindings (`tests/test_c_extension.xb`, `tests/test_raylib.xb`)

**Result: 52 / 52 Passed (100%)**.

---

## 5. Using the Grammar

This PEG grammar can be integrated directly with parser generators across multiple ecosystems:
- **C/C++**: `cpp-peglib`, `tree-sitter`, or `packrat`
- **Rust**: `pest` or `nom`
- **Python**: `pyparsing` or `TatSu`
- **JavaScript/TypeScript**: `Peggy` (`pegjs`) or `Ohm`
