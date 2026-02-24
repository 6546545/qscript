use crate::ir::{BasicBlock, Instruction, IrValue, ModuleIr};

/// Placeholder classical backend that prints a pseudo-LLVM view of the IR.
pub fn emit(module: &ModuleIr) {
    println!("== QScript Classical Backend (pseudo-LLVM IR) ==");
    for func in &module.functions {
        println!("define @{}() {{", func.name);
        for block in &func.blocks {
            println!("  {}:", block.name);
            for inst in &block.instructions {
                match inst {
                    Instruction::Nop => println!("    ; nop"),
                    Instruction::Call { callee, .. } => println!("    ; call {}", callee),
                    Instruction::Ret => println!("    ; ret"),
                }
            }
        }
        println!("}}");
    }
}

fn escape_llvm_string(s: &str) -> String {
    let mut out = String::new();
    for c in s.chars() {
        match c {
            '\\' => out.push_str("\\\\"),
            '"' => out.push_str("\\22"),
            '\n' => out.push_str("\\0A"),
            '\r' => out.push_str("\\0D"),
            '\t' => out.push_str("\\09"),
            _ if c.is_ascii() && (c as u8) >= 32 => out.push(c),
            _ => out.push_str(&format!("\\{:02X}", c as u32)),
        }
    }
    out
}

/// Emit valid LLVM IR (text) to stdout. Pipe to clang to produce an executable.
/// For main() with print("...") emits a call to printf so the program prints the string.
pub fn emit_llvm(module: &ModuleIr) {
    println!("target triple = \"x86_64-unknown-unknown\"");
    println!("target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"");

    // Declare printf; use format "%s\n" and pass string as second arg
    println!("declare i32 @printf(i8*, ...)");
    println!("@.str.fmt = private unnamed_addr constant [4 x i8] c\"%s\\0A\\00\", align 1");

    // Collect string constants and emit globals
    let mut str_consts: Vec<String> = Vec::new();
    for func in &module.functions {
        for block in &func.blocks {
            for inst in &block.instructions {
                if let Instruction::Call { callee, args } = inst {
                    if callee == "print" {
                        for a in args {
                            if let IrValue::Str(s) = a {
                                str_consts.push(s.clone());
                            }
                        }
                    }
                }
            }
        }
    }
    for (idx, s) in str_consts.iter().enumerate() {
        let escaped = escape_llvm_string(s);
        let len = s.chars().count() + 1;
        println!(
            "@.str.{} = private unnamed_addr constant [{} x i8] c\"{}\\00\", align 1",
            idx, len, escaped
        );
    }

    let mut str_idx = 0;
    for func in &module.functions {
        let ret_ty = if func.name == "main" { "i32" } else { "void" };
        println!("define {} @{}() {{", ret_ty, func.name);
        println!("entry:");
        for block in &func.blocks {
            for inst in &block.instructions {
                match inst {
                    Instruction::Nop => {}
                    Instruction::Ret => {
                        if func.name == "main" {
                            println!("  ret i32 0");
                        } else {
                            println!("  ret void");
                        }
                    }
                    Instruction::Call { callee, args } => {
                        if callee == "print" && args.len() == 1 {
                            if let IrValue::Str(_) = &args[0] {
                                let i = str_idx;
                                str_idx += 1;
                                let n = str_consts[i].chars().count() + 1;
                                println!(
                                    "  %str.ptr = getelementptr inbounds ([{} x i8], [{} x i8]* @.str.{}, i64 0, i64 0)",
                                    n, n, i
                                );
                                println!("  %fmt.ptr = getelementptr inbounds ([4 x i8], [4 x i8]* @.str.fmt, i64 0, i64 0)");
                                println!("  %1 = call i32 @printf(i8* %fmt.ptr, i8* %str.ptr)");
                            }
                        }
                    }
                }
            }
        }
        println!("}}");
    }
}

