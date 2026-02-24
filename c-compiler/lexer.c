#include "lexer.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_whitespace(const char *p) {
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        break;
    }
    return p;
}

static int is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_ident_cont(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

static int is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '<' || c == '>' || c == '=' || c == '!' || c == '&' || c == '|';
}

static TokenKind keyword_kind(const char *start, size_t len) {
    if (len == 2 && memcmp(start, "fn", 2) == 0) return TOK_FN;
    if (len == 3 && memcmp(start, "let", 3) == 0) return TOK_LET;
    if (len == 2 && memcmp(start, "if", 2) == 0) return TOK_IF;
    if (len == 4 && memcmp(start, "else", 4) == 0) return TOK_ELSE;
    if (len == 4 && memcmp(start, "loop", 4) == 0) return TOK_LOOP;
    if (len == 7 && memcmp(start, "quantum", 7) == 0) return TOK_QUANTUM;
    if (len == 6 && memcmp(start, "return", 6) == 0) return TOK_RETURN;
    if (len == 5 && memcmp(start, "break", 5) == 0) return TOK_BREAK;
    return TOK_IDENTIFIER;
}

#define TOK_BUF_INIT 32

typedef struct {
    Token *data;
    size_t len;
    size_t cap;
} TokenBuf;

static int buf_push(TokenBuf *buf, Token t) {
    if (buf->len >= buf->cap) {
        size_t new_cap = buf->cap ? buf->cap * 2 : TOK_BUF_INIT;
        Token *n = (Token *)realloc(buf->data, new_cap * sizeof(Token));
        if (!n) return -1;
        buf->data = n;
        buf->cap = new_cap;
    }
    buf->data[buf->len++] = t;
    return 0;
}

static char *strndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

int lex(const char *source, Token **out_tokens, size_t *out_count) {
    TokenBuf buf = { NULL, 0, 0 };
    const char *p = source;

    while (1) {
        p = skip_whitespace(p);
        if (!*p) break;

        if (is_ident_start(*p)) {
            const char *start = p;
            while (is_ident_cont(*p)) p++;
            size_t len = (size_t)(p - start);
            TokenKind kind = keyword_kind(start, len);
            char *value = (kind == TOK_IDENTIFIER) ? strndup(start, len) : NULL;
            Token t = { kind, value };
            if (buf_push(&buf, t) != 0) goto fail;
            continue;
        }

        if (isdigit((unsigned char)*p)) {
            const char *start = p;
            while (isdigit((unsigned char)*p)) p++;
            char *value = strndup(start, (size_t)(p - start));
            if (!value) goto fail;
            Token t = { TOK_INTEGER_LITERAL, value };
            if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            continue;
        }

        if (*p == '"') {
            const char *start = ++p;
            while (*p && *p != '"') {
                if (*p == '\\') { p++; if (!*p) goto fail; }
                p++;
            }
            if (!*p) goto fail; /* unterminated string */
            char *value = strndup(start, (size_t)(p - start));
            if (!value) goto fail;
            p++; /* consume closing " */
            Token t = { TOK_STRING_LITERAL, value };
            if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            continue;
        }

        if (p[0] == '-' && p[1] == '>') {
            Token t = { TOK_ARROW, NULL };
            if (buf_push(&buf, t) != 0) goto fail;
            p += 2;
            continue;
        }

        if (*p == '{') { Token t = { TOK_LBRACE, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == '}') { Token t = { TOK_RBRACE, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == '(') { Token t = { TOK_LPAREN, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == ')') { Token t = { TOK_RPAREN, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == '[') { Token t = { TOK_LBRACKET, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == ']') { Token t = { TOK_RBRACKET, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == '<') { Token t = { TOK_LANGLE, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == '>') { Token t = { TOK_RANGLE, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == ':') { Token t = { TOK_COLON, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == ';') { Token t = { TOK_SEMICOLON, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }
        if (*p == ',') { Token t = { TOK_COMMA, NULL }; if (buf_push(&buf, t) != 0) goto fail; p++; continue; }

        if (is_operator_char(*p)) {
            const char *start = p;
            while (is_operator_char(*p)) p++;
            char *value = strndup(start, (size_t)(p - start));
            if (!value) goto fail;
            Token t = { TOK_OPERATOR, value };
            if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            continue;
        }

        goto fail; /* unexpected character */
    }

    {
        Token t = { TOK_EOF, NULL };
        if (buf_push(&buf, t) != 0) goto fail;
    }

    *out_tokens = buf.data;
    *out_count = buf.len;
    return 0;

fail:
    lex_free(buf.data, buf.len);
    return -1;
}

void lex_free(Token *tokens, size_t count) {
    if (!tokens) return;
    for (size_t i = 0; i < count; i++)
        free(tokens[i].value);
    free(tokens);
}
