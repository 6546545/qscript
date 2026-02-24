## Research Summary: Intermediate Representation (IR) Design

This document summarizes key research ideas and best practices in **intermediate representation design** for compilers targeting heterogeneous systems (classical + quantum).

### SSA and Static Single Assignment

- **SSA form** is a widely used representation (notably in LLVM) where each variable is assigned exactly once.
- Benefits:
  - Simplifies many analyses (liveness, reaching definitions).
  - Makes optimization passes (constant propagation, dead-code elimination) more straightforward.
  - Works well as a base for both scalar and aggregate data transformations.

QScript’s IR will use **SSA for classical data** to align with these practices and ease translation to LLVM IR.

### Multi-Level and Multi-Dialect IRs

- **MLIR** and related research introduce the idea of:
  - Multiple **dialects** for different abstraction levels or domains (e.g., high-level linear algebra vs. low-level loops).
  - Progressive lowering from high-level dialects to low-level ones and ultimately to machine code.
- This is especially useful in heterogeneous systems (CPUs, GPUs, quantum devices) where:
  - Different operations have different constraints and semantics.

QScript takes inspiration from this by:

- Providing a **core IR** that captures classical control and data.
- Extending it with a **quantum instruction family** (allocate qubit, apply gate, measure, reset).

### Quantum IR Considerations

- Gate-level quantum IRs (e.g., OpenQASM, QIR) operate on:
  - Qubit identifiers.
  - A sequence of gates and measurements with well-defined ordering.
- Compiler passes may perform:
  - Gate fusion, cancellation, and reordering under constraints.
  - Mapping logical qubits to physical qubits (layout).
  - Insertion of error-mitigation or error-correction structures.

QScript’s IR will:

- Represent quantum instructions in a way that can be **lowered cleanly** to external quantum IRs.
- Keep track of **source-level information** (file, line, column) for diagnostics.

### Metadata and Optimization

- IRs often attach metadata (debug info, optimization hints, attributes) to:
  - Functions, basic blocks, and instructions.
  - Support better diagnostics and profile-guided optimization.

QScript will support **metadata attachments** in its IR to enable:

- Better error reporting.
- Future optimization passes that leverage profiling or user-supplied hints.

