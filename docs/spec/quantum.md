## QScript Quantum Model (v0.1)

This document outlines the initial quantum execution model for QScript and how it integrates with the classical core.

### Quantum Types

Core quantum-related types:

- `qubit` – a single quantum bit.
- `qreg<N>` – a fixed-size register of `N` qubits.
- `qarray` – a dynamically sized collection of qubits (may be introduced later; v0.1 focuses on fixed-size registers).

These types are treated as **linear resources**:

- They cannot be implicitly copied.
- They must be consumed, passed, or measured according to linear usage rules enforced by the type checker.

### Quantum Operations

Primitive operations include:

- **Allocation**:
  - Allocate one or more qubits or registers in a fresh state (e.g., `|0⟩` states).
- **Gate application**:
  - Apply unitary gates (e.g., `H`, `X`, `Y`, `Z`, `CX`, `CZ`, `RX`, `RY`, `RZ`, parameterized rotations).
- **Measurement**:
  - Measure a qubit or register, producing classical bits and collapsing the quantum state.
- **Reset**:
  - Reset a qubit to a known basis state (usually `|0⟩`), if supported by the backend.

These operations are represented explicitly in the IR for quantum lowering.

### Classical–Quantum Boundary

QScript code will have **explicit quantum blocks** that delineate when quantum operations are allowed. Within such a block:

- Qubits and registers may be allocated, entangled, and measured.
- Classical variables can be read to influence which gates are applied (e.g., controlled by classical flags).
- Measurement results become classical values that can be used after the block.

Outside quantum blocks, direct quantum operations are not permitted; qubits cannot be referenced or manipulated except via well-defined APIs.

### Linearity and Safety

To respect the **no-cloning theorem** and general quantum safety:

- `qubit` and `qreg<N>` are **linearly typed**:
  - A binding must be consumed exactly once, passed along, or measured/reset under rules enforced by the type checker.
- Passing a qubit to a function transfers ownership unless an explicit borrowing construct for quantum data is introduced (a potential v0.2+ feature).

These rules will be encoded in the type system and enforced by the compiler frontend before IR generation.

