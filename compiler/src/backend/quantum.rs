use crate::ir::ModuleIr;

/// Placeholder quantum backend that prints a stub quantum IR header.
pub fn emit_stub(module: &ModuleIr) {
    println!("== QScript Quantum Backend (stub quantum IR) ==");
    for func in &module.functions {
        println!("; quantum view for function {}", func.name);
    }
}

