# QScript Quantum Backend (MVP)

This document describes the mechanical mapping from QScript's quantum instructions to **OpenQASM 2.0** for the MVP.

## Mapping Overview

| QScript (MVP) | OpenQASM 2.0 |
|---------------|--------------|
| `alloc_qreg<2>()` | `qreg q[2];` |
| `h(q[0])` | `h q[0];` |
| `cx(q[0], q[1])` | `cx q[0],q[1];` |
| `measure_all(q)` | `measure q -> c;` (with `creg c[2];`) |
| `print_bits(result)` | (classical; no QASM equivalent) |

## Bell Pair Example

QScript:

```qs
quantum {
    let q: qreg<2> = alloc_qreg<2>();
    h(q[0]);
    cx(q[0], q[1]);
    let result = measure_all(q);
    print_bits(result);
}
```

Emitted OpenQASM 2.0:

```qasm
OPENQASM 2.0;
include "qelib1.inc";
qreg q[2];
creg c[2];
h q[0];
cx q[0],q[1];
measure q -> c;
```

## Simulator Workflow

1. Compile: `qlangc --qasm -o bell.qasm examples/bell_pair.qs`
2. Run with Qiskit (Python): `python scripts/run_qasm.py bell.qasm`
3. Or use IBM Quantum Lab, Qiskit Aer, or other OpenQASM 2.0–compatible simulators.

Expected Bell pair outcome: approximately 50% `00` and 50% `11` on measurement.
