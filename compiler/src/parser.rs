use crate::ast::{Function, Program};
use crate::lexer::{Token, TokenKind};

#[derive(Debug)]
pub enum ParseError {
    UnexpectedEnd,
}

pub fn parse(tokens: &[Token]) -> Result<Program, ParseError> {
    // Extremely minimal placeholder parser: collect every identifier following `fn` as a function.
    let mut functions = Vec::new();
    let mut i = 0;
    while i + 1 < tokens.len() {
        if let TokenKind::Fn = tokens[i].kind {
            if let TokenKind::Identifier(name) = &tokens[i + 1].kind {
                functions.push(Function { name: name.clone() });
            }
        }
        i += 1;
    }
    Ok(Program { functions })
}

