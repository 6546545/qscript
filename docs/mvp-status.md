# QScript MVP Status

This page summarizes what works in the **Minimal Viable Product (MVP)** release.

## Implemented

- **Specs**: Core, quantum, and syntax specs with explicit MVP scope and future-work sections.
- **Compiler**: C-based implementation in `c-compiler/` (lexer, parser, AST, typechecker, IR, backends).
- **Classical backend**: Compiles `hello_world.qs` to native executable (via C emission or LLVM IR).
- **Quantum backend**: Emits OpenQASM for `bell_pair.qs`; documented simulator workflow.
- **Examples**: `examples/hello_world.qs`, `examples/bell_pair.qs`, `examples/greet.qs`, `examples/conditional.qs`, `examples/arithmetic.qs`, `examples/loop.qs`, `examples/return_value.qs`, `examples/assignment.qs`, `examples/for_loop.qs`, `examples/while_loop.qs`.

## MVP Limitations

- Only `unit`, `i32`, `qreg<2>` types.
- `main()` must have no parameters; other functions may have parameters.
- `if`/`else` supported with comparison conditions (`<`, `>`, `==`, `!=`, `<=`, `>=`).
- `let` bindings lowered to classical IR (alloca/store); variables usable in conditions and calls.
- Arithmetic in let init: `+`, `-`, `*`, `/`, `%`.
- `return;` and `return expr;` (early exit; `-> i32` functions require `return expr;`).
- `loop { ... }` and `break;` for control flow.
- Mutable assignment: `x = expr;` (variable must be in scope from `let` or param).
- `while (cond) { ... }`, `for (init; cond; step) { ... }`, `continue;`.
- No structs, pointers, or advanced memory model.
- Quantum: only `alloc_qreg<2>`, `h`, `cx`, `measure_all`, `print_bits`.
- **Swarms**: `scripts/swarm.py` provides unified CLI for spec, example, and DX workflows.

## Items to Do (Planned)

- (All planned control flow implemented.)
- Quantum backend from IR instead of hardcoding.

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
