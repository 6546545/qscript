## QScript Design Vision

QScript is a new low-level yet readable programming language designed to bridge **high-performance classical computing** and **emerging quantum hardware** in a way that is approachable for everyday programmers while grounded in serious computer science research.

### Implementation Language Decision

The **reference compiler, core tooling, and runtime glue code are implemented in Rust**. This decision is based on the following factors:

- **Low-level control with safety**: Rust exposes memory layout, borrowing, and lifetimes while maintaining strong safety guarantees, making it ideal for building a low-level language implementation without sacrificing robustness.
- **Performance**: Rust compiles to efficient native code and integrates well with LLVM, which we rely on for classical backends.
- **Ecosystem**: Mature tooling (Cargo, testing, formatting, profiling) and libraries for systems programming, CLI tooling, and FFI.
- **Cross-platform support**: Rust supports Windows, Linux, and macOS with good ergonomics, which aligns with QScript’s deployment goals.

Rust is **an implementation choice only**; QScript’s syntax and semantics are designed independently and are **not derived from Rust**.

### Core Goals

- **Low-level control**: Give programmers explicit control over memory, data layout, and performance characteristics suitable for systems programming and graphics-heavy workloads.
- **Readability and usability**: Favor a clean, consistent syntax oriented toward everyday programmers, with clear error messages and strong tooling.
- **Hybrid classical–quantum model**: Provide first-class language constructs for qubits, gates, measurement, and classical–quantum interaction, informed by current quantum programming research.
- **Research-driven design**: Align language features and internals (type system, IR, backends) with published research where possible, documenting influences and trade-offs.
- **Open source and extensible**: Encourage contributions, experiments with alternative backends, and integration with external tools.

This document will evolve as the language and implementation mature, but Rust as the primary implementation language is the firm baseline for the reference toolchain.

