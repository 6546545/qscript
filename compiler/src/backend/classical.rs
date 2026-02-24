use crate::ir::ModuleIr;

/// Placeholder classical backend that prints a pseudo-LLVM view of the IR.
pub fn emit(module: &ModuleIr) {
    println!("== QScript Classical Backend (pseudo-LLVM IR) ==");
    for func in &module.functions {
        println!("define @{}() {{", func.name);
        for block in &func.blocks {
            println!("  {}:", block.name);
            for _inst in &block.instructions {
                println!("    ; nop");
            }
        }
        println!("}}");
    }
}

