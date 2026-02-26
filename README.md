## QScript Language (Working Title)

QScript is an experimental **low-level yet readable programming language** designed from first principles to:

- Target both **classical CPUs** and **quantum processors**.
- Support **high-performance, graphics-heavy** applications and simple programs like "Hello, world!".
- Be grounded in **computer science research** on type systems, IR design, and quantum programming.
- Provide a **modern developer experience** with good tooling and documentation.

### Implementation Overview (MVP)

- **Compiler**: Implemented in **C** in `c-compiler/` (lexer, parser, AST, typechecker, IR, backends).
- **Backends**:
  - Classical: SSA-style IR lowered to **LLVM IR**; compiles to native via Clang.
  - Quantum: Emits **OpenQASM 2.0** for quantum circuits (e.g., Bell pair).
- **Examples**: `examples/hello_world.qs`, `examples/bell_pair.qs`, `examples/greet.qs`, `examples/conditional.qs`, `examples/else_if.qs`, `examples/arithmetic.qs`, `examples/loop.qs`, `examples/return_value.qs`, `examples/assignment.qs`, `examples/for_loop.qs`, `examples/while_loop.qs`, `examples/print_int.qs`, `examples/logical_and_or.qs`, `examples/bool_type.qs`, `examples/type_alias.qs`, `examples/unary_minus.qs`, `examples/hex_and_escapes.qs`, `examples/parentheses.qs`, `examples/parentheses_simple.qs`.
- **Swarms**: Optional Python workflows in `orchestration/` for spec drafting and example generation.

### Quickstart

**Prerequisites**: GCC or Clang (C compiler), and Clang for linking LLVM IR to native.

1. **Build the compiler**:
   ```bash
   cd c-compiler
   make
   # Or on Windows: .\build.ps1
   ```

2. **Run hello_world** (classical):
   ```bash
   ./qlangc -o hello ../examples/hello_world.qs
   ./hello
   # Output: Hello, world!
   ```

3. **Emit OpenQASM for bell_pair** (quantum):
   ```bash
   ./qlangc --qasm -o bell.qasm ../examples/bell_pair.qs
   python ../scripts/run_qasm.py bell.qasm
   # Requires: pip install qiskit qiskit-aer
   ```

**Alternative** (emit LLVM IR manually):
```bash
./qlangc --llvm -o out.ll ../examples/hello_world.qs
clang -x ir out.ll -o hello
./hello
```

### MVP Limitations

- Types: `unit`, `i32`, `bool`, `qreg<2>`; type aliases: `type Foo = i32;`. Comments: `//` and `/* */`. Integer literals: decimal, hex (`0x1a`), binary (`0b1010`), octal (`0o77`). String escapes: `\n`, `\t`, `\"`, `\\`.
- `main()` must have no parameters; other functions may have parameters.
- `if`/`else`/`else if` supported (comparison conditions, `&&`, `||`, `!`).
- `let` bindings with variables usable in conditions and calls; arithmetic (`+`, `-`, `*`, `/`, `%`) and unary minus (`-1`, `-x`) in init.
- `return;` and `return expr;` for early exit; functions can return `-> i32`.
- `loop { ... }` and `break;` for control flow.
- Mutable assignment: `x = expr;`
- `while`, `for`, `continue`.
- Quantum: only `alloc_qreg<2>`, `h`, `cx`, `measure_all`, `print_bits`.
- See `docs/mvp-status.md` and `docs/language-tour-mvp.md` for details.

### Planned (Items to Do)

- (Control flow complete.)

### CLI Reference

| Option | Description |
|--------|-------------|
| `qlangc <file>` | Parse, typecheck, print pseudo-IR |
| `qlangc -o <binary> <file>` | Compile to native executable |
| `qlangc --llvm [-o out.ll] <file>` | Emit LLVM IR |
| `qlangc --qasm` / `--emit-qasm` `[-o out.qasm] <file>` | Emit OpenQASM 2.0 |
| `qlangc --tokens <file>` | Dump token stream |

### Swarms Workflows (Optional)

AI-assisted workflows for spec refinement, example generation, and error message improvement.

**Install**: `pip install swarms`

**Run** (unified CLI from project root):
```bash
python scripts/swarm.py spec "quantum type system"    # Spec refinement
python scripts/swarm.py example "fn main() -> unit { let x = 1 + 2; }"  # Generate examples
python scripts/swarm.py dx "undefined variable x"     # Improve error messages
```

See `orchestration/README.md` for details.

### Quick test

From project root, build and run the hello_world example:

```powershell
# Windows
.\scripts\run_tests.ps1
```

```bash
# Linux / macOS
./scripts/run_tests.sh
```

### IDE (VS Code Extension)

A basic IDE is available in `ide/vscode-extension/`:

- Syntax highlighting, snippets, bracket matching
- Build (`Ctrl+Shift+B`) and Run commands
- Diagnostics via LSP (parse/type errors)

See `ide/README.md` for setup. Requires VS Code and the QScript compiler.

### Repository Layout

- `c-compiler/` – Compiler implementation (lexer, parser, AST, typechecker, IR, classical and quantum backends).
- `ide/` – VS Code extension for QScript (syntax, build/run, LSP).
- `docs/` – Specs, design docs, MVP status, language tour.
- `examples/` – `hello_world.qs`, `bell_pair.qs`, `greet.qs`, `conditional.qs`, `arithmetic.qs`, `loop.qs`, `return_value.qs`, `assignment.qs`, and others.
- `orchestration/` – Swarms workflows.
- `scripts/` – `run_qasm.py` (QASM simulator), `run_tests.ps1` / `run_tests.sh` (build and sanity test).
- `tools/lsp/` – QScript Language Server (diagnostics).

