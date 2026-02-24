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
- **Examples**: `examples/hello_world.qs`, `examples/bell_pair.qs`.
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

- Types: `unit`, `i32`, `qreg<2>` only.
- No `if`/`else`, `loop`, `while`, `for`.
- Quantum: only `alloc_qreg<2>`, `h`, `cx`, `measure_all`, `print_bits`.
- See `docs/mvp-status.md` and `docs/language-tour-mvp.md` for details.

### CLI Reference

| Option | Description |
|--------|-------------|
| `qlangc <file>` | Parse, typecheck, print pseudo-IR |
| `qlangc -o <binary> <file>` | Compile to native executable |
| `qlangc --llvm [-o out.ll] <file>` | Emit LLVM IR |
| `qlangc --qasm` / `--emit-qasm` `[-o out.qasm] <file>` | Emit OpenQASM 2.0 |
| `qlangc --tokens <file>` | Dump token stream |

### Swarms Workflows (Optional)

The `orchestration/` folder contains [swarms](https://github.com/kyegomez/swarms?tab=readme-ov-file)-based workflows for spec refinement and example generation.

**Install**:
```bash
pip install swarms
```

**Run**:
```bash
# Propose spec refinements for a topic
python orchestration/spec_swarm.py "quantum type system"

# Generate examples from spec text
python orchestration/example_swarm.py "fn main() -> unit { ... }"

# DX swarm (see orchestration/dx_swarm.py)
python orchestration/dx_swarm.py
```

### Repository Layout

- `c-compiler/` – C compiler implementation (canonical for MVP).
- `compiler/` – Legacy Rust prototype.
- `docs/` – Specs, design docs, MVP status, language tour.
- `examples/` – `hello_world.qs`, `bell_pair.qs`.
- `orchestration/` – Swarms workflows.
- `scripts/` – `run_qasm.py` for running emitted QASM on a simulator.

