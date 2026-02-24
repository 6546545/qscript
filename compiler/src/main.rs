mod lexer;
mod ast;
mod parser;
mod typecheck;
mod ir;
mod backend;

use clap::Parser as ClapParser;

/// Simple CLI for the QScript compiler.
#[derive(ClapParser, Debug)]
#[command(name = "qlangc", about = "QScript reference compiler (prototype)")]
struct Cli {
    /// Path to the source file to compile.
    #[arg()]
    input: String,
}

fn main() {
    let cli = Cli::parse();
    let source = std::fs::read_to_string(&cli.input)
        .expect("failed to read source file");

    let tokens = lexer::lex(&source);
    let ast = parser::parse(&tokens).expect("parse error");
    let _typed = typecheck::typecheck(&ast).expect("type error");

    let ir_module = ir::lower_to_ir(&ast);

    // For now, always emit both classical and quantum IR stubs.
    backend::classical::emit(&ir_module);
    backend::quantum::emit_stub(&ir_module);

    println!("Compiled {}", cli.input);
}

