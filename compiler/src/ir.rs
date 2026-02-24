use crate::ast::{Expr, Function, Program, Statement};

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
pub enum IrValue {
    Str(String),
    Int(String),
}

#[derive(Debug)]
pub enum Instruction {
    Nop,
    Call { callee: String, args: Vec<IrValue> },
    Ret,
}

pub fn lower_to_ir(program: &Program) -> ModuleIr {
    let functions = program
        .functions
        .iter()
        .map(|f| {
            let mut instructions = Vec::new();
            for stmt in &f.body {
                if let Statement::Call { callee, args } = stmt {
                    let ir_args: Vec<IrValue> = args
                        .iter()
                        .map(|e| match e {
                            Expr::StringLit(s) => IrValue::Str(s.clone()),
                            Expr::IntLit(s) => IrValue::Int(s.clone()),
                            Expr::Identifier(s) => IrValue::Int(s.clone()), // placeholder
                        })
                        .collect();
                    instructions.push(Instruction::Call {
                        callee: callee.clone(),
                        args: ir_args,
                    });
                }
            }
            instructions.push(Instruction::Ret);
            if instructions.len() == 1 {
                instructions.insert(0, Instruction::Nop);
            }
            FunctionIr {
                name: f.name.clone(),
                blocks: vec![BasicBlock {
                    name: "entry".to_string(),
                    instructions,
                }],
            }
        })
        .collect();

    ModuleIr { functions }
}
