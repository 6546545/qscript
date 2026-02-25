# QScript Language Tour (MVP)

This page walks through the MVP examples to illustrate the language subset.

## Hello, World!

```qs
fn main() -> unit {
    print("Hello, world!");
}
```

- **`fn main() -> unit`**: Function named `main` with no parameters, returning `unit` (void).
- **`print("Hello, world!")`**: Built-in that prints a string to stdout.
- **`print_int(n)`**: Built-in that prints an integer to stdout.
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

## Functions with Parameters

```qs
fn greet(msg: i32) -> unit {
    print("Greeting received");
}

fn main() -> unit {
    greet(42);
}
```

- **`fn greet(msg: i32) -> unit`**: Function with one parameter `msg` of type `i32`.
- **`greet(42)`**: Call with an integer literal argument.
- **Note**: `main()` must have no parameters; other functions may have parameters.

---

## Arithmetic and Return

```qs
fn main() -> unit {
    let x: i32 = 1 + 2;
    if x < 5 {
        print("ok");
        return;
    }
    print("fallthrough");
}
```

- **`let x = 1 + 2`**: Arithmetic operators `+`, `-`, `*`, `/`, `%` in let initializers. Unary minus: `let n = -1;`, `let m = -x;`.
- **`return;`**: Early exit from a function. Skips remaining statements.

---

## Function Return Values

```qs
fn add(a: i32, b: i32) -> i32 {
    return a + b;
}

fn main() -> unit {
    let x: i32 = add(1, 2);
    if x < 5 {
        print("x is less than 5");
    }
}
```

- **`-> i32`**: Function returns an integer. Must use `return expr;` (not `return;`).
- **`let x = add(1, 2)`**: Call expressions can be used in let initializers when the function returns `i32`.

---

## Conditionals (if/else)

```qs
fn main() -> unit {
    if 1 < 2 {
        print("yes");
    } else {
        print("no");
    }
}
```

- **`if expr { ... } else if expr { ... } else { ... }`**: Conditional with comparison. Supported operators: `<`, `>`, `<=`, `>=`, `==`, `!=`, logical `&&`, `||`, and unary `!`.
- Variables from `let` bindings can be used in conditions and function calls.

---

## Loop and Break

```qs
fn main() -> unit {
    loop {
        print("Loop iteration");
        break;
    }
    print("Done");
}
```

- **`loop { ... }`**: Infinite loop. Body executes until `break` or `return`.
- **`break;`**: Exits the innermost loop. Must be inside a `loop` block.

---

## Mutable Assignment

```qs
fn main() -> unit {
    let i: i32 = 0;
    loop {
        if i >= 3 { break; }
        print("iteration");
        i = i + 1;
    }
    print("done");
}
```

- **`x = expr;`**: Assigns a new value to an existing variable. The variable must be in scope (from `let` or a function parameter).

---

## Bool Type

```qs
fn main() -> unit {
    let a: bool = true;
    let b: bool = false;
    if (a && !b) {
        print_int(1);
    }
}
```

- **`bool`**: Boolean type. Represented as `i32` (0 or 1) in the backend.
- **`true`**, **`false`**: Boolean literals.
- **`let x: bool = true`**: Variables can be explicitly typed as `bool`.
- **`let c: bool = 1 < 2`**: Comparisons produce a value (0 or 1), so they can be assigned to `bool` or used in expressions.
- **`if (a)`**: Bool variables can be used directly in conditions.

---

## MVP Syntax Summary

| Construct | Example |
|-----------|---------|
| Function | `fn name() -> unit { ... }` or `fn name(x: i32, y: i32) -> unit { ... }` |
| Let + arithmetic | `let x = 1 + 2;` |
| Conditional | `if a < b { ... } else { ... }` (supports `&&`, `||`, `!`) |
| Loop | `loop { ... break; }` |
| Assignment | `x = expr;` |
| Return | `return;` or `return expr;` (for `-> i32`) |
| Let binding | `let x = expr;` or `let x: type = expr;` |
| Quantum block | `quantum { ... }` |
| Call | `print("hi");` or `print_int(x);` |
| Index | `q[0]` |
| Types | `unit`, `i32`, `bool`, `qreg<2>` |

---

## What's Not in the MVP

- Swarms integration and support (deeper orchestration with compiler/tooling).
- (Control flow: `while`, `for`, `continue` implemented.)
- Generics and complex types
- Additional quantum gates beyond `h` and `cx`
- Full linearity/ownership for quantum types
