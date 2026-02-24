## QScript Surface Syntax (v0.1)

This document describes the initial **surface syntax** of QScript for simple programs, focusing on clarity and readability rather than similarity to any existing language.

### Basic Structure

A QScript source file consists of:

- Optional metadata (to be defined later).
- One or more `fn` declarations.
- Optional `struct` declarations and type aliases.

### Hello World Example

```text
fn main() -> unit {
    print(\"Hello, world!\");
}
```

Key points:

- `fn` introduces a function.
- `main` is the entry point for simple programs.
- `unit` is the explicit return type for functions returning no meaningful value.
- Statements are terminated by `;`.

### Variables and Bindings

```text
fn main() -> unit {
    let x: i32 = 42;
    let y: i32 = x + 1;
    print_int(y);
}
```

- `let` introduces a new binding.
- Types are written after `:` and are usually required in v0.1 for clarity.

### Conditionals

```text
fn compare(a: i32, b: i32) -> i32 {
    if a < b {
        return -1;
    } else {
        if a > b {
            return 1;
        } else {
            return 0;
        }
    }
}
```

### Loops

```text
fn sum_to(n: i32) -> i32 {
    let total: i32 = 0;
    let i: i32 = 0;
    loop {
        if i > n {
            break;
        }
        total = total + i;
        i = i + 1;
    }
    return total;
}
```

### Simple Quantum Example (Sketch)

```text
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

Here, `quantum { ... }` introduces a quantum block where quantum operations (`alloc_qreg`, `h`, `cx`, `measure_all`) are permitted.

This syntax is intentionally minimal for v0.1 and will evolve as the language and tooling mature.

