#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TokenKind {
    Fn,
    Let,
    If,
    Else,
    Loop,
    Quantum,
    Return,
    Identifier(String),
    IntegerLiteral(String),
    StringLiteral(String),
    LBrace,
    RBrace,
    LParen,
    RParen,
    LAngle,
    RAngle,
    LBracket,
    RBracket,
    Colon,
    Semicolon,
    Comma,
    Arrow,
    Operator(String),
}

#[derive(Debug, Clone)]
pub struct Token {
    pub kind: TokenKind,
}

pub fn lex(source: &str) -> Vec<Token> {
    // Very minimal placeholder lexer that splits on whitespace and treats
    // each chunk as an identifier or integer literal.
    source
        .split_whitespace()
        .map(|chunk| {
            let kind = if chunk == "fn" {
                TokenKind::Fn
            } else if chunk == "let" {
                TokenKind::Let
            } else if chunk == "if" {
                TokenKind::If
            } else if chunk == "else" {
                TokenKind::Else
            } else if chunk == "loop" {
                TokenKind::Loop
            } else if chunk == "quantum" {
                TokenKind::Quantum
            } else if chunk == "return" {
                TokenKind::Return
            } else if chunk.chars().all(|c| c.is_ascii_digit()) {
                TokenKind::IntegerLiteral(chunk.to_string())
            } else {
                TokenKind::Identifier(chunk.to_string())
            };
            Token { kind }
        })
        .collect()
}

