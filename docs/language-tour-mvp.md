# QScript Language Tour (MVP)

This page walks through the MVP examples to illustrate the language subset.

## Hello, World!

```qs
fn main() -> unit {
    print("Hello, world!");
}
```

- **`fn main() -> unit`**: Function named `main` with no parameters, returning `unit` (void).
- **`print("Hello, world!")`**: Built-in function that prints a string to stdout.
- Every program must define `main() -> unit` as the entry point.

**Compile and run**:
```bash
qlangc -o hello examples/hello_world.qs
./hello
```

---

## Bell Pair (Quantum)

```qs
fn main() -> unit {
    bell_pair();
}

fn bell_pair() -> unit {
    quantum {
        let q: qreg<2> = alloc_qreg<2>();
        h(q[0]);
        cx(q[0], q[1]);
        let result = measure_all(q);
        print_bits(result);
    }
}
```

- **`quantum { ... }`**: Block where quantum operations are allowed.
- **`let q: qreg<2> = alloc_qreg<2>()`**: Allocate a 2-qubit register in state |00⟩.
- **`h(q[0])`**: Apply Hadamard gate to qubit 0.
- **`cx(q[0], q[1])`**: Apply CNOT with control q[0], target q[1]. Creates a Bell pair.
- **`measure_all(q)`**: Measure all qubits; returns a classical result.
- **`print_bits(result)`**: Print the measurement outcome.

**Compile and run**:
```bash
qlangc --qasm -o bell.qasm examples/bell_pair.qs
python scripts/run_qasm.py bell.qasm
```

Expected: ~50% `00`, ~50% `11` (correlated Bell pair outcomes).

---

## MVP Syntax Summary

| Construct | Example |
|-----------|---------|
| Function | `fn name() -> unit { ... }` |
| Let binding | `let x = expr;` or `let x: type = expr;` |
| Quantum block | `quantum { ... }` |
| Call | `print("hi");` |
| Index | `q[0]` |
| Types | `unit`, `i32`, `qreg<2>` |

---

## What's Not in the MVP

- Control flow: `if`, `else`, `loop`, `for`, `while`
- Function parameters
- Generics and complex types
- Additional quantum gates beyond `h` and `cx`
- Full linearity/ownership for quantum types
