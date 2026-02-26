#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_whitespace(const char *p) {
    for (;;) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        if (p[0] == '/' && p[1] == '*') {
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/')) p++;
            if (*p) p += 2;
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
    if (len == 3 && memcmp(start, "for", 3) == 0) return TOK_FOR;
    if (len == 5 && memcmp(start, "while", 5) == 0) return TOK_WHILE;
    if (len == 7 && memcmp(start, "quantum", 7) == 0) return TOK_QUANTUM;
    if (len == 6 && memcmp(start, "return", 6) == 0) return TOK_RETURN;
    if (len == 5 && memcmp(start, "break", 5) == 0) return TOK_BREAK;
    if (len == 8 && memcmp(start, "continue", 8) == 0) return TOK_CONTINUE;
    if (len == 4 && memcmp(start, "true", 4) == 0) return TOK_TRUE;
    if (len == 5 && memcmp(start, "false", 5) == 0) return TOK_FALSE;
    if (len == 4 && memcmp(start, "type", 4) == 0) return TOK_TYPE;
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
            unsigned long long num = 0;
            if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
                p += 2;
                if (!*p || !isxdigit((unsigned char)*p)) goto fail;
                while (isxdigit((unsigned char)*p)) {
                    unsigned int d = (unsigned char)*p - '0';
                    if (d > 9) d = (unsigned char)(*p | 32) - 'a' + 10;
                    num = num * 16 + d;
                    p++;
                }
                char dec[32];
                (void)snprintf(dec, sizeof(dec), "%llu", num);
                char *value = strdup(dec);
                if (!value) goto fail;
                Token t = { TOK_INTEGER_LITERAL, value };
                if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            } else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
                p += 2;
                if (!*p || (*p != '0' && *p != '1')) goto fail;
                while (*p == '0' || *p == '1') {
                    num = num * 2 + (unsigned long long)(*p - '0');
                    p++;
                }
                char dec[32];
                (void)snprintf(dec, sizeof(dec), "%llu", num);
                char *value = strdup(dec);
                if (!value) goto fail;
                Token t = { TOK_INTEGER_LITERAL, value };
                if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            } else if (p[0] == '0' && (p[1] == 'o' || p[1] == 'O')) {
                p += 2;
                if (!*p || *p < '0' || *p > '7') goto fail;
                while (*p >= '0' && *p <= '7') {
                    num = num * 8 + (unsigned long long)(*p - '0');
                    p++;
                }
                char dec[32];
                (void)snprintf(dec, sizeof(dec), "%llu", num);
                char *value = strdup(dec);
                if (!value) goto fail;
                Token t = { TOK_INTEGER_LITERAL, value };
                if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            } else {
                while (isdigit((unsigned char)*p)) p++;
                char *value = strndup(start, (size_t)(p - start));
                if (!value) goto fail;
                Token t = { TOK_INTEGER_LITERAL, value };
                if (buf_push(&buf, t) != 0) { free(value); goto fail; }
            }
            continue;
        }

        if (*p == '"') {
            p++;
            size_t cap = 64, len = 0;
            char *value = (char *)malloc(cap);
            if (!value) goto fail;
            while (*p && *p != '"') {
                if (len >= cap) {
                    cap *= 2;
                    char *n = (char *)realloc(value, cap);
                    if (!n) { free(value); goto fail; }
                    value = n;
                }
                if (*p == '\\') {
                    p++;
                    if (!*p) { free(value); goto fail; }
                    switch (*p) {
                        case 'n': value[len++] = '\n'; break;
                        case 't': value[len++] = '\t'; break;
                        case 'r': value[len++] = '\r'; break;
                        case '"': value[len++] = '"'; break;
                        case '\\': value[len++] = '\\'; break;
                        default: value[len++] = *p; break;
                    }
                    p++;
                } else {
                    value[len++] = *p++;
                }
            }
            if (!*p) { free(value); goto fail; } /* unterminated string */
            p++; /* consume closing " */
            value[len] = '\0';
            char *v = (char *)realloc(value, len + 1);
            if (v) value = v;
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
        if (p[0] == '<' && p[1] == '=') { Token t = { TOK_OPERATOR, strndup("<=", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
        if (p[0] == '>' && p[1] == '=') { Token t = { TOK_OPERATOR, strndup(">=", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
        if (p[0] == '=' && p[1] == '=') { Token t = { TOK_OPERATOR, strndup("==", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
        if (p[0] == '!' && p[1] == '=') { Token t = { TOK_OPERATOR, strndup("!=", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
        if (p[0] == '!' && p[1] != '=') { Token t = { TOK_OPERATOR, strndup("!", 1) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p++; continue; }
        if (p[0] == '&' && p[1] == '&') { Token t = { TOK_OPERATOR, strndup("&&", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
        if (p[0] == '|' && p[1] == '|') { Token t = { TOK_OPERATOR, strndup("||", 2) }; if (!t.value || buf_push(&buf, t) != 0) { if (t.value) free(t.value); goto fail; } p += 2; continue; }
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
