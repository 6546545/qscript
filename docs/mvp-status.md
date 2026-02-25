# QScript MVP Status

This page summarizes what works in the **Minimal Viable Product (MVP)** release.

## Implemented

- **Specs**: Core, quantum, and syntax specs with explicit MVP scope and future-work sections.
- **Compiler**: C-based implementation in `c-compiler/` (lexer, parser, AST, typechecker, IR, backends).
- **Classical backend**: Compiles `hello_world.qs` to native executable (via C emission or LLVM IR).
- **Quantum backend**: Emits OpenQASM for `bell_pair.qs`; documented simulator workflow.
- **Examples**: `examples/hello_world.qs`, `examples/bell_pair.qs`, `examples/greet.qs`, `examples/conditional.qs`.

## MVP Limitations

- Only `unit`, `i32`, `qreg<2>` types.
- `main()` must have no parameters; other functions may have parameters.
- `if`/`else` supported with comparison conditions (`<`, `>`, `==`, `!=`, `<=`, `>=`).
- No `loop`, `while`, `for`.
- No structs, pointers, or advanced memory model.
- Quantum: only `alloc_qreg<2>`, `h`, `cx`, `measure_all`, `print_bits`.

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
