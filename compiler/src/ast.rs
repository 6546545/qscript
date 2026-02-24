#[derive(Debug)]
pub struct Program {
    pub functions: Vec<Function>,
}

#[derive(Debug)]
pub struct Function {
    pub name: String,
    pub body: Vec<Statement>,
}

#[derive(Debug)]
pub enum Statement {
    Call { callee: String, args: Vec<Expr> },
}

#[derive(Debug)]
pub enum Expr {
    StringLit(String),
    IntLit(String),
    Identifier(String),
}
