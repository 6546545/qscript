# QScript MVP Status

This page summarizes what works in the **Minimal Viable Product (MVP)** release.

## Implemented

- **Specs**: Core, quantum, and syntax specs with explicit MVP scope and future-work sections.
- **Compiler**: C-based implementation in `c-compiler/` (lexer, parser, AST, typechecker, IR, backends).
- **Classical backend**: Compiles `hello_world.qs` to native executable (via C emission or LLVM IR).
- **Quantum backend**: Emits OpenQASM for `bell_pair.qs`; documented simulator workflow.
- **Examples**: `examples/hello_world.qs`, `examples/bell_pair.qs`, `examples/greet.qs`, `examples/conditional.qs`, `examples/else_if.qs`, `examples/arithmetic.qs`, `examples/loop.qs`, `examples/return_value.qs`, `examples/assignment.qs`, `examples/for_loop.qs`, `examples/while_loop.qs`, `examples/print_int.qs`, `examples/logical_and_or.qs`, `examples/bool_type.qs`, `examples/type_alias.qs`, `examples/unary_minus.qs`, `examples/hex_and_escapes.qs`, `examples/parentheses.qs`, `examples/parentheses_simple.qs`.

## MVP Limitations

- Types: `unit`, `i32`, `bool`, `qreg<2>`.
- `main()` must have no parameters; other functions may have parameters.
- `if`/`else`/`else if` supported with comparison conditions (`<`, `>`, `==`, `!=`, `<=`, `>=`), logical `&&` and `||` (short-circuit), and unary `!` (logical not).
- `let` bindings lowered to classical IR (alloca/store); variables usable in conditions and calls. Comparisons can be used as values: `let c: bool = 1 < 2;`.
- Arithmetic in let init: `+`, `-`, `*`, `/`, `%`; unary minus: `-1`, `-x`. Parenthesized expressions: `(1 + 2) * 3`. Integer literals: decimal, hex (`0x1a`), binary (`0b1010`), octal (`0o77`).
- String literals support escape sequences: `\n`, `\t`, `\r`, `\"`, `\\`.
- `return;` and `return expr;` (early exit; `-> i32` functions require `return expr;`).
- `loop { ... }` and `break;` for control flow.
- Mutable assignment: `x = expr;` (variable must be in scope from `let` or param).
- `while (cond) { ... }`, `for (init; cond; step) { ... }`, `continue;`.
- **Type aliases**: `type Name = i32;` (or `unit`, `bool`, `qreg<2>`) at top level; aliases are resolved in function signatures and `let` annotations. Duplicate alias names and unknown RHS types are rejected.
- **Comments**: `//` line comments and `/* ... */` block comments.
- No structs, pointers, or advanced memory model.
- `print("...")` and `print_int(i32)` built-ins.
- Quantum: only `alloc_qreg<2>`, `h`, `cx`, `measure_all`, `print_bits`.
- **Swarms**: `scripts/swarm.py` provides unified CLI for spec, example, and DX workflows.

## Items to Do (Planned)

- (All planned control flow implemented.)
- (Quantum backend from IR implemented.)

## Release checklist

Before publishing (e.g. tagging `v0.1.0` or creating a GitHub Release):

1. Build the compiler with no warnings: `make` (Unix) or `.\build.ps1` (Windows) in `c-compiler/`.
2. Run the sanity test from project root: `./scripts/run_tests.sh` (Unix) or `.\scripts\run_tests.ps1` (Windows).
3. Optionally: compile and run a few more examples (e.g. `arithmetic.qs`, `parentheses.qs`) and run `qlangc --qasm -o bell.qasm examples/bell_pair.qs` to verify the quantum backend.

## Build and Run

See `README.md` for quickstart. In brief:

```bash
cd c-compiler
make
./qlangc -o hello ../examples/hello_world.qs
./hello
./qlangc --qasm -o bell.qasm ../examples/bell_pair.qs
python ../scripts/run_qasm.py bell.qasm
```

**Quick sanity test** (from project root): `./scripts/run_tests.sh` (Unix) or `.\scripts\run_tests.ps1` (Windows).
