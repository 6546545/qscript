use crate::ast::{Function, Program};

#[derive(Debug)]
pub struct ModuleIr {
    pub functions: Vec<FunctionIr>,
}

#[derive(Debug)]
pub struct FunctionIr {
    pub name: String,
    pub blocks: Vec<BasicBlock>,
}

#[derive(Debug)]
pub struct BasicBlock {
    pub name: String,
    pub instructions: Vec<Instruction>,
}

#[derive(Debug)]
pub enum Instruction {
    Nop,
    // Placeholders for future classical and quantum instructions.
    // E.g., Add, Sub, Call, QuantumAlloc, QuantumGate, Measure, etc.
}

pub fn lower_to_ir(program: &Program) -> ModuleIr {
    let functions = program
        .functions
        .iter()
        .map(|f| FunctionIr {
            name: f.name.clone(),
            blocks: vec![BasicBlock {
                name: "entry".to_string(),
                instructions: vec![Instruction::Nop],
            }],
        })
        .collect();

    ModuleIr { functions }
}

