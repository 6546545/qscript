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

fn is_ident_start(c: char) -> bool {
    c.is_ascii_alphabetic() || c == '_'
}

fn is_ident_cont(c: char) -> bool {
    is_ident_start(c) || c.is_ascii_digit()
}

fn is_operator_char(c: char) -> bool {
    matches!(c, '+' | '-' | '*' | '/' | '%' | '<' | '>' | '=' | '!' | '&' | '|')
}

fn keyword_kind(s: &str) -> Option<TokenKind> {
    match s {
        "fn" => Some(TokenKind::Fn),
        "let" => Some(TokenKind::Let),
        "if" => Some(TokenKind::If),
        "else" => Some(TokenKind::Else),
        "loop" => Some(TokenKind::Loop),
        "quantum" => Some(TokenKind::Quantum),
        "return" => Some(TokenKind::Return),
        _ => None,
    }
}

pub fn lex(source: &str) -> Vec<Token> {
    let mut tokens = Vec::new();
    let mut chars = source.char_indices().peekable();

    while let Some((_, c)) = chars.peek().copied() {
        if c.is_ascii_whitespace() {
            chars.next();
            continue;
        }
        if c == '/' && chars.clone().nth(1).map(|(_, d)| d) == Some('/') {
            chars.next();
            chars.next();
            while chars.peek().map(|(_, d)| *d != '\n').unwrap_or(false) {
                chars.next();
            }
            continue;
        }

        if is_ident_start(c) {
            let start = chars.next().unwrap().0;
            while chars.peek().map(|(_, d)| is_ident_cont(*d)).unwrap_or(false) {
                chars.next();
            }
            let end = chars.peek().map(|(i, _)| *i).unwrap_or(source.len());
            let s = &source[start..end];
            let kind = keyword_kind(s)
                .unwrap_or_else(|| TokenKind::Identifier(s.to_string()));
            tokens.push(Token { kind });
            continue;
        }

        if c.is_ascii_digit() {
            let start = chars.next().unwrap().0;
            while chars.peek().map(|(_, d)| d.is_ascii_digit()).unwrap_or(false) {
                chars.next();
            }
            let end = chars.peek().map(|(i, _)| *i).unwrap_or(source.len());
            tokens.push(Token {
                kind: TokenKind::IntegerLiteral(source[start..end].to_string()),
            });
            continue;
        }

        if c == '"' {
            chars.next(); // consume opening "
            let start = chars.peek().map(|(i, _)| *i).unwrap_or(source.len());
            let mut end = start;
            while let Some((i, d)) = chars.next() {
                if d == '\\' {
                    chars.next();
                    end = chars.peek().map(|(j, _)| *j).unwrap_or(source.len());
                    continue;
                }
                if d == '"' {
                    break;
                }
                end = i + d.len_utf8();
            }
            tokens.push(Token {
                kind: TokenKind::StringLiteral(source[start..end].to_string()),
            });
            continue;
        }

        if c == '-' && chars.clone().nth(1).map(|(_, d)| d) == Some('>') {
            chars.next();
            chars.next();
            tokens.push(Token { kind: TokenKind::Arrow });
            continue;
        }

        let kind = match c {
            '{' => TokenKind::LBrace,
            '}' => TokenKind::RBrace,
            '(' => TokenKind::LParen,
            ')' => TokenKind::RParen,
            '[' => TokenKind::LBracket,
            ']' => TokenKind::RBracket,
            '<' => TokenKind::LAngle,
            '>' => TokenKind::RAngle,
            ':' => TokenKind::Colon,
            ';' => TokenKind::Semicolon,
            ',' => TokenKind::Comma,
            _ if is_operator_char(c) => {
                let start = chars.next().unwrap().0;
                while chars
                    .peek()
                    .map(|(_, d)| is_operator_char(*d))
                    .unwrap_or(false)
                {
                    chars.next();
                }
                let end = chars.peek().map(|(i, _)| *i).unwrap_or(source.len());
                TokenKind::Operator(source[start..end].to_string())
            }
            _ => {
                chars.next();
                continue;
            }
        };

        if !matches!(kind, TokenKind::Operator(_)) {
            chars.next();
        }
        tokens.push(Token { kind });
    }

    tokens
}
