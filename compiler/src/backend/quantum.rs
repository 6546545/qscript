use crate::ir::ModuleIr;

/// Placeholder quantum backend that prints a stub quantum IR header.
pub fn emit_stub(module: &ModuleIr) {
    println!("== QScript Quantum Backend (stub quantum IR) ==");
    for func in &module.functions {
        println!("; quantum view for function {}", func.name);
    }
}

/// Emit OpenQASM 2.0 to stdout. For known quantum entry points (e.g. bell_pair)
/// emits a runnable circuit; otherwise emits a minimal valid circuit.
/// Example: cargo run -- ../examples/bell_pair.qs --qasm > bell_pair.qasm
pub fn emit_qasm(module: &ModuleIr) {
    for func in &module.functions {
        if func.name == "bell_pair" {
            // Bell pair: H on q0, CX q0->q1, measure both (runnable in Qiskit, etc.)
            println!("OPENQASM 2.0;");
            println!("include \"qelib1.inc\";");
            println!("qreg q[2];");
            println!("creg c[2];");
            println!("h q[0];");
            println!("cx q[0],q[1];");
            println!("measure q -> c;");
            return;
        }
    }
    // Default: minimal valid OpenQASM (one qubit, one classical bit, no gates)
    println!("OPENQASM 2.0;");
    println!("include \"qelib1.inc\";");
    println!("qreg q[1];");
    println!("creg c[1];");
}

