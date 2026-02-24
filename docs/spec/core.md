## QScript Core Semantics (v0.1)

This document defines the initial core semantics of the QScript language, focusing on the **classical execution model** while anticipating integration with quantum features.

### Values and Types

Primitive value types in v0.1:

- `i8`, `i16`, `i32`, `i64` – signed integers.
- `u8`, `u16`, `u32`, `u64` – unsigned integers.
- `f32`, `f64` – IEEE-754 floating-point numbers.
- `bool` – boolean values (`true`, `false`).
- `unit` – no meaningful value (analogous to `void`), written as `()`.

Compound types:

- **Pointers**: `ptr<T>` – raw, unsafe pointer to `T`.
- **Refs**: `ref<T>` – safe, borrow-checked reference to `T` (non-owning).
- **Arrays**: `[T; N]` – fixed-size array of N elements of type `T`.
- **Slices**: `slice<T>` – view into a contiguous sequence of `T` with runtime length.
- **Structs**: `struct Name { field: Type, ... }`.

### Memory and Ownership (Classical)

QScript employs an **ownership-based memory model** for stack-allocated and heap-allocated values, inspired by research on affine/linear and ownership type systems:

- Each value has a **single owning location** at any point.
- **Moves** transfer ownership; copies are only allowed for explicitly copyable types (e.g., small integers, `bool`).
- **Borrows** create `ref<T>` references that:
  - Cannot outlive the owner.
  - Cannot be mutated if there is an active immutable borrow (and vice versa).

Explicit heap allocation is provided via standard library functions (e.g., `alloc`, `free` equivalents) rather than built-in syntax in v0.1.

### Control Flow

Core structured control flow constructs:

- **Blocks**: delimited regions introducing scopes for variables.
- **Conditionals**: `if` / `else` constructs with expression or statement bodies.
- **Loops**:
  - `loop { ... }` – infinite loop with explicit `break` / `continue`.
  - `while` and `for` constructs may be added as sugar over `loop` in later iterations.

Blocks form the basis for **basic blocks** in the compiler IR.

### Functions and Modules

Functions:

- Declared with a clear signature: parameter list with types, return type.
- Support pass-by-value parameters; references must be annotated as `ref<T>`.
- May be pure or impure; effect tracking is a future extension.

Modules:

- Files act as modules; imports and exports will be specified in a separate module system document, but v0.1 assumes a flat module structure suitable for single-file programs and simple `examples/`.

### Errors and Diagnostics

The reference compiler must:

- Detect **type errors**, **ownership/borrow violations**, and **obvious undefined behaviors** (e.g., use-before-initialization).
- Emit diagnostics with:
  - Source span (file, line, column).
  - Short error code.
  - Human-readable explanation and, when possible, suggestions.

These semantics provide the classical foundation on which the quantum model and richer type system features will be integrated.

