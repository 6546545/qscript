## Research Summary: Low-Level Language Design

This document summarizes influential ideas from computer science research on **low-level, high-performance language design** that inform QScript.

### Memory Models and Safety

- **Ownership and borrowing (Rust, Cyclone, linear/affine type systems)**:
  - Enforce at-most-one mutable reference and aliasing restrictions at compile time.
  - Reduce data races and use-after-free errors without a tracing GC.
  - Relevant papers: work on affine/linear types and region-based memory management.
- **Region-based memory management**:
  - Allocate objects into regions with well-defined lifetimes that can be reclaimed in bulk.
  - Helps with predictable deallocation and can improve cache locality.
- **Explicit layout and alignment**:
  - Allow programmers to control struct layout, alignment, and packing for FFI and performance-critical code.

QScript leans toward an **ownership/borrowing-inspired model** for safety, with explicit escape hatches where full control is required.

### Type Systems and Effects

- **Strong static typing with type inference**:
  - Reduce boilerplate while maintaining guarantees about memory and control flow.
- **Effect systems**:
  - Track side effects (I/O, mutation, quantum operations) in the type system.
  - Can help separate pure from impure code and reason about quantum/classical boundaries.
- **Linear and affine types**:
  - Originating from linear logic, enforce single-use or limited-use semantics for resources.
  - Directly useful for modeling non-duplicable resources like qubits.

QScript’s type system will incorporate **linearity/affinity for resources** and may expose an effect-like notion for quantum operations.

### Control Flow and Data-Oriented Design

- **Structured control flow**:
  - Loops, conditionals, and pattern matching are preferred over unrestricted gotos for readability and optimizability.
- **Data-oriented design (DoD)**:
  - Emphasizes struct-of-arrays, contiguous storage, and cache-friendly patterns.
  - Influential in game engines and high-performance graphics.

QScript emphasizes **structured control flow** and **data-oriented patterns** as first-class citizens for performance-sensitive code, especially for graphics and numerical kernels.

