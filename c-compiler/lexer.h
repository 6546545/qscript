#ifndef QSCRIPT_LEXER_H
#define QSCRIPT_LEXER_H

#include <stddef.h>

typedef enum {
    TOK_FN,
    TOK_LET,
    TOK_IF,
    TOK_ELSE,
    TOK_LOOP,
    TOK_QUANTUM,
    TOK_RETURN,
    TOK_BREAK,
    TOK_IDENTIFIER,
    TOK_INTEGER_LITERAL,
    TOK_STRING_LITERAL,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LANGLE,
    TOK_RANGLE,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_ARROW,
    TOK_OPERATOR,
    TOK_EOF,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    char *value; /* owned; non-NULL only for IDENTIFIER, INTEGER_LITERAL, STRING_LITERAL, OPERATOR */
} Token;

/**
 * Lex the entire source into a list of tokens.
 * Returns 0 on success, -1 on error (e.g. unterminated string).
 * On success, *out_tokens is set to a newly allocated array and *out_count to its length.
 * Caller must free with lex_free().
 */
int lex(const char *source, Token **out_tokens, size_t *out_count);

void lex_free(Token *tokens, size_t count);

#endif /* QSCRIPT_LEXER_H */
