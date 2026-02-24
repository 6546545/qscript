## QScript Intermediate Representation (IR) (v0.1)

QScript uses a unified SSA-style IR to represent both classical and (eventually) quantum operations.

### Module Structure

- **Module**:
  - Contains a list of functions.
- **Function**:
  - Has a name.
  - Contains one or more basic blocks.
- **Basic block**:
  - Has a label (e.g., `entry`).
  - Contains a sequence of instructions.

### Instructions

In v0.1, the instruction set is intentionally minimal:

- `Nop` – placeholder for no operation.

Future iterations will add:

- Classical operations:
  - Arithmetic (`Add`, `Sub`, etc.).
  - Control flow (`Br`, `CondBr`, `Ret`).
  - Memory (`Load`, `Store`, `Alloca`).
- Quantum operations:
  - Qubit allocation (`QuantumAlloc`).
  - Gate application (`QuantumGate`).
  - Measurement (`Measure`).

The IR will serve as the bridge between the frontend (parser/typechecker) and the classical/quantum backends.

