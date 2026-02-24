## Research Summary: Quantum Programming Languages

This document summarizes key ideas from research on **quantum programming languages** that shape QScript’s quantum model.

### Existing Quantum Languages and IRs

- **Q# (Microsoft)**:
  - High-level quantum language with a rich standard library and strong tooling.
  - Emphasizes separation of classical control and quantum operations.
  - Introduces concepts like adjoint and controlled variants of operations.
- **OpenQASM (IBM)**:
  - Assembly-like language/IR for quantum circuits, widely supported by tooling and hardware.
  - Good target for expressing gate-level circuits and measurements.
- **Quipper, Silq, and others**:
  - Explore higher-level abstractions, automatic uncomputation, and safer handling of quantum data.
- **QIR (Quantum Intermediate Representation)**:
  - LLVM-based IR for quantum programs, designed for backend-agnostic quantum compilation and optimization.

QScript will **not** mimic any of these syntactically, but it will **reuse proven semantic ideas**, such as clear separation of classical control from quantum ops and support for adjoint/controlled forms.

### No-Cloning and Linearity

- Quantum information cannot, in general, be copied (`no-cloning theorem`), which motivates **linear type systems**:
  - Qubits must be consumed exactly once or according to strict rules.
  - Measurements convert quantum data into classical bits, after which the quantum state is no longer available.
- Research on **quantum lambda calculi** and linear logic provides a formal foundation for such constraints.

QScript’s type system will use **linear (or affine) typing for qubits and quantum registers**, ensuring correct usage at compile time wherever possible.

### Classical–Quantum Interaction

- Common patterns in research and practice:
  - Classical code prepares parameters, controls loop structures, and interprets measurement results.
  - Quantum subsections perform gate sequences on qubits and return measurement outcomes.
- Many languages encode this as a **host–device model** or as special blocks/sections for quantum code.

QScript will offer **explicit constructs for entering a quantum context**, allocating qubits, applying operations, and returning classical data, while preserving a clear boundary between classical and quantum effects.

