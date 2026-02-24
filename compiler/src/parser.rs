use crate::ast::{Expr, Function, Program, Statement};
use crate::lexer::{Token, TokenKind};

#[derive(Debug)]
pub enum ParseError {
    UnexpectedEnd,
    Expected(&'static str),
}

pub fn parse(tokens: &[Token]) -> Result<Program, ParseError> {
    let mut functions = Vec::new();
    let mut i = 0;

    while i < tokens.len() {
        // Skip to next fn or end
        while i < tokens.len() && !matches!(tokens[i].kind, TokenKind::Fn) {
            i += 1;
        }
        if i >= tokens.len() {
            break;
        }
        i += 1; // consume fn

        let name = match &tokens.get(i).ok_or(ParseError::UnexpectedEnd)?.kind {
            TokenKind::Identifier(s) => s.clone(),
            _ => return Err(ParseError::Expected("function name")),
        };
        i += 1;

        if !matches!(tokens.get(i), Some(Token { kind: TokenKind::LParen }) ) {
            return Err(ParseError::Expected("'('"));
        }
        i += 1;
        if !matches!(tokens.get(i), Some(Token { kind: TokenKind::RParen }) ) {
            return Err(ParseError::Expected("')'"));
        }
        i += 1;
        if !matches!(tokens.get(i), Some(Token { kind: TokenKind::Arrow }) ) {
            return Err(ParseError::Expected("'->'"));
        }
        i += 1;
        // return type: identifier or identifier < ... >
        if matches!(tokens.get(i), Some(Token { kind: TokenKind::Identifier(_) }) ) {
            i += 1;
        }
        if matches!(tokens.get(i), Some(Token { kind: TokenKind::LAngle }) ) {
            i += 1;
            let mut depth = 1u32;
            while i < tokens.len() && depth > 0 {
                match &tokens[i].kind {
                    TokenKind::LAngle => depth += 1,
                    TokenKind::RAngle => {
                        depth -= 1;
                        if depth == 0 {
                            i += 1;
                            break;
                        }
                    }
                    _ => {}
                }
                i += 1;
            }
        }

        if !matches!(tokens.get(i), Some(Token { kind: TokenKind::LBrace }) ) {
            return Err(ParseError::Expected("'{'"));
        }
        i += 1;

        let mut body = Vec::new();
        while i < tokens.len() {
            if matches!(tokens[i].kind, TokenKind::RBrace) {
                i += 1;
                break;
            }
            // statement: identifier ( arg ) ;
            if let Some(Token { kind: TokenKind::Identifier(callee) }) = tokens.get(i) {
                let callee = callee.clone();
                i += 1;
                if matches!(tokens.get(i), Some(Token { kind: TokenKind::LParen }) ) {
                    i += 1;
                    let mut args = Vec::new();
                    while !matches!(tokens.get(i), Some(Token { kind: TokenKind::RParen }) ) {
                        let expr = parse_expr(tokens, &mut i)?;
                        args.push(expr);
                        if matches!(tokens.get(i), Some(Token { kind: TokenKind::Comma }) ) {
                            i += 1;
                        }
                    }
                    i += 1; // consume )
                    if matches!(tokens.get(i), Some(Token { kind: TokenKind::Semicolon }) ) {
                        i += 1;
                        body.push(Statement::Call { callee, args });
                        continue;
                    }
                }
                // not a call; rewind and skip to next ;
                i -= 1;
            }
            // skip until ; or }
            while i < tokens.len() {
                if matches!(tokens[i].kind, TokenKind::Semicolon | TokenKind::RBrace) {
                    if matches!(tokens[i].kind, TokenKind::Semicolon) {
                        i += 1;
                    }
                    break;
                }
                i += 1;
            }
        }

        functions.push(Function { name, body });
    }

    Ok(Program { functions })
}

fn parse_expr(tokens: &[Token], i: &mut usize) -> Result<Expr, ParseError> {
    match tokens.get(*i) {
        Some(Token { kind: TokenKind::StringLiteral(s) }) => {
            let s = s.clone();
            *i += 1;
            Ok(Expr::StringLit(s))
        }
        Some(Token { kind: TokenKind::IntegerLiteral(s) }) => {
            let s = s.clone();
            *i += 1;
            Ok(Expr::IntLit(s))
        }
        Some(Token { kind: TokenKind::Identifier(s) }) => {
            let s = s.clone();
            *i += 1;
            Ok(Expr::Identifier(s))
        }
        _ => Err(ParseError::Expected("expression")),
    }
}
