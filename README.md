## QScript Language (Working Title)

QScript is an experimental **low-level yet readable programming language** designed from first principles to:

- Target both **classical CPUs** and **quantum processors**.
- Support **high-performance, graphics-heavy** applications and simple programs like \"Hello, world!\".
- Be grounded in **computer science research** on type systems, IR design, and quantum programming.
- Provide a **modern developer experience** with good tooling and documentation.

### Implementation Overview

- **Compiler & core tooling**: Implemented in **Rust** to get low-level control, performance, and memory safety.
- **Backends**:
  - Classical: via a custom SSA-style IR lowered to **LLVM IR** for portability across x86_64 / ARM64 and major OSes.
  - Quantum: via a quantum-aware IR lowered to formats like **OpenQASM** or **QIR**, integrating with vendor SDKs.
- **Runtime**:
  - Minimal classical runtime and standard library primitives.
  - Quantum runtime shim that bridges language-level quantum operations to quantum SDKs or simulators.
- **Multi-agent orchestration**: Uses the open source [`swarms`](https://github.com/kyegomez/swarms?tab=readme-ov-file) framework to assist with:
  - Research ingestion and summarization.
  - Drafting and reviewing language specifications.
  - Generating examples, tests, and documentation.

### Repository Layout (Planned)

- `compiler/` – Rust compiler, IR, and backends.
- `runtime/` – Classical and quantum runtime components.
- `docs/` – Language specification, design docs, and research summaries.
- `examples/` – Sample programs from \"Hello, world!\" to graphics and quantum demos.
- `orchestration/` – Python-based `swarms` workflows that support research, spec drafting, and DX.

This repository is in an **early experimental phase**. The initial goal is to ship a minimal end-to-end toolchain:

1. Parse and type-check a small core of the language.
2. Emit classical machine code via LLVM for basic examples.
3. Emit quantum IR for simple quantum programs (e.g., Bell pair) runnable via external tooling.

### CLI Usage

Once Rust and Cargo are installed, you can run the prototype compiler:

```bash
cd compiler
cargo run -- ../examples/hello_world.qs
```

By default this parses and type-checks the source and prints **pseudo-IR** for both classical and quantum backends.

**Emit real LLVM IR** (then compile to an executable with [Clang](https://clang.llvm.org/)):

```bash
cargo run -- ../examples/hello_world.qs --llvm | clang -x ir - -o hello
./hello
```

**Emit OpenQASM 2.0** for quantum programs (e.g. Bell pair), then run with [Qiskit](https://qiskit.org/) or another OpenQASM-capable tool:

```bash
cargo run -- ../examples/bell_pair.qs --qasm > bell_pair.qasm
# Example with Qiskit: python -c "from qiskit import QuantumCircuit; qc = QuantumCircuit.from_qasm_file('bell_pair.qasm'); print(qc)"
```

The implementation parses and type-checks a small subset of the language; the classical backend can emit valid LLVM IR and the quantum backend can emit runnable OpenQASM for the Bell pair example.

