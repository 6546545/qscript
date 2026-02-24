## QScript Design Vision

QScript is a new low-level yet readable programming language designed to bridge **high-performance classical computing** and **emerging quantum hardware** in a way that is approachable for everyday programmers while grounded in serious computer science research.

### Implementation Language Decision

The **reference compiler and runtime glue code for the MVP are implemented in C**. This decision is based on the following factors:

- **Low-level control**: C provides direct control over memory and data layout, ideal for building a compiler that targets both classical and quantum backends.
- **Portability**: C compiles on virtually every platform; a simple Makefile or build script suffices for Windows, Linux, and macOS.
- **LLVM integration**: The LLVM C API allows direct integration for classical code generation without additional language runtimes.
- **Minimal dependencies**: No package manager required; contributors can build with `gcc`/`clang` and standard tooling.

C is **an implementation choice only**; QScript's syntax and semantics are designed independently and are **not derived from C** (or any other language). The compiler lives in `c-compiler/`; the legacy Rust prototype in `compiler/` may be retained for reference but is not the canonical MVP implementation.

### Core Goals

- **Low-level control**: Give programmers explicit control over memory, data layout, and performance characteristics suitable for systems programming and graphics-heavy workloads.
- **Readability and usability**: Favor a clean, consistent syntax oriented toward everyday programmers, with clear error messages and strong tooling.
- **Hybrid classical-quantum model**: Provide first-class language constructs for qubits, gates, measurement, and classical-quantum interaction, informed by current quantum programming research.
- **Research-driven design**: Align language features and internals (type system, IR, backends) with published research where possible, documenting influences and trade-offs.
- **Open source and extensible**: Encourage contributions, experiments with alternative backends, and integration with external tools.

This document will evolve as the language and implementation mature. For the MVP, C in `c-compiler/` is the canonical compiler implementation.
