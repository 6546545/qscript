#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_INIT 8
#define ERR_MAX  128

static char parse_last_error[ERR_MAX];

const char *parse_get_last_error(void) {
    return parse_last_error;
}

static void set_error(const char *msg) {
    (void)snprintf(parse_last_error, sizeof(parse_last_error), "%s", msg);
}

static int peek(const Token *tokens, size_t token_count, size_t *i, TokenKind kind) {
    while (*i < token_count && tokens[*i].kind == TOK_EOF) (*i)++;
    if (*i >= token_count) return 0;
    return tokens[*i].kind == kind;
}

static int consume(const Token *tokens, size_t token_count, size_t *i, TokenKind kind) {
    if (!peek(tokens, token_count, i, kind)) return 0;
    (*i)++;
    return 1;
}

/* Forward declarations */
static int parse_expr(const Token *tokens, size_t token_count, size_t *i, Expr *e);
static int parse_stmt(const Token *tokens, size_t token_count, size_t *i,
                      Statement **out_stmt);

/* Parse primary: string | int | ident | ident<integer> */
static int parse_primary(const Token *tokens, size_t token_count, size_t *i, Expr *e) {
    if (*i >= token_count) return 0;
    const Token *t = &tokens[*i];
    memset(e, 0, sizeof(Expr));
    if (t->kind == TOK_STRING_LITERAL && t->value) {
        e->kind = EXPR_STR;
        e->value = strdup(t->value);
        if (!e->value) return 0;
        (*i)++;
        return 1;
    }
    if (t->kind == TOK_INTEGER_LITERAL && t->value) {
        e->kind = EXPR_INT;
        e->value = strdup(t->value);
        if (!e->value) return 0;
        (*i)++;
        return 1;
    }
    if (t->kind == TOK_IDENTIFIER && t->value) {
        e->kind = EXPR_IDENT;
        e->value = strdup(t->value);
        if (!e->value) return 0;
        (*i)++;
        /* ident<integer> for alloc_qreg<2>, qreg<2> etc */
        if (*i < token_count && tokens[*i].kind == TOK_LANGLE) {
            size_t j = *i + 1;
            if (j < token_count && tokens[j].kind == TOK_INTEGER_LITERAL && tokens[j].value) {
                size_t k = j + 1;
                if (k < token_count && tokens[k].kind == TOK_RANGLE) {
                    size_t base_len = strlen(e->value);
                    size_t num_len = strlen(tokens[j].value);
                    char *combined = (char *)malloc(base_len + 1 + num_len + 1 + 1);
                    if (!combined) return 0;
                    snprintf(combined, base_len + num_len + 4, "%s<%s>", e->value, tokens[j].value);
                    free(e->value);
                    e->value = combined;
                    *i = k + 1;
                }
            }
        }
        return 1;
    }
    return 0;
}

/* Parse expression: primary postfix* where postfix = [ expr ] | ( args ) */
static int parse_expr(const Token *tokens, size_t token_count, size_t *i, Expr *e) {
    Expr primary;
    if (!parse_primary(tokens, token_count, i, &primary)) return 0;
    *e = primary;

    while (*i < token_count) {
        if (peek(tokens, token_count, i, TOK_LBRACKET)) {
            (*i)++;
            Expr idx;
            if (!parse_expr(tokens, token_count, i, &idx)) {
                ast_free_expr(e);
                return 0;
            }
            if (!consume(tokens, token_count, i, TOK_RBRACKET)) {
                ast_free_expr(&idx);
                ast_free_expr(e);
                return 0;
            }
            Expr *base = (Expr *)malloc(sizeof(Expr));
            if (!base) { ast_free_expr(&idx); ast_free_expr(e); return 0; }
            *base = *e;
            memset(e, 0, sizeof(Expr));
            e->kind = EXPR_INDEX;
            e->base = base;
            e->index = (Expr *)malloc(sizeof(Expr));
            if (!e->index) { free(base); ast_free_expr(&idx); return 0; }
            *e->index = idx;
            continue;
        }
        if (peek(tokens, token_count, i, TOK_LPAREN)) {
            (*i)++;
            Expr *args = NULL;
            size_t arg_count = 0, arg_cap = 0;
            if (!peek(tokens, token_count, i, TOK_RPAREN)) {
                do {
                    Expr arg;
                    if (!parse_expr(tokens, token_count, i, &arg)) {
                        if (args) {
                            for (size_t k = 0; k < arg_count; k++) ast_free_expr(&args[k]);
                            free(args);
                        }
                        ast_free_expr(e);
                        return 0;
                    }
                    if (arg_count >= arg_cap) {
                        size_t new_cap = arg_cap ? arg_cap * 2 : 4;
                        Expr *n = (Expr *)realloc(args, new_cap * sizeof(Expr));
                        if (!n) {
                            ast_free_expr(&arg);
                            if (args) {
                                for (size_t k = 0; k < arg_count; k++) ast_free_expr(&args[k]);
                                free(args);
                            }
                            ast_free_expr(e);
                            return 0;
                        }
                        args = n;
                        arg_cap = new_cap;
                    }
                    args[arg_count++] = arg;
                } while (consume(tokens, token_count, i, TOK_COMMA));
            }
            if (!consume(tokens, token_count, i, TOK_RPAREN)) {
                if (args) {
                    for (size_t k = 0; k < arg_count; k++) ast_free_expr(&args[k]);
                    free(args);
                }
                ast_free_expr(e);
                return 0;
            }
            Expr *callee = (Expr *)malloc(sizeof(Expr));
            if (!callee) {
                if (args) {
                    for (size_t k = 0; k < arg_count; k++) ast_free_expr(&args[k]);
                    free(args);
                }
                ast_free_expr(e);
                return 0;
            }
            *callee = *e;
            memset(e, 0, sizeof(Expr));
            e->kind = EXPR_CALL;
            e->base = callee;
            e->args = args;
            e->arg_count = arg_count;
            continue;
        }
        break;
    }
    return 1;
}

/* Parse one statement; *out_stmt is allocated and filled. Return 1 on success. */
static int parse_stmt(const Token *tokens, size_t token_count, size_t *i,
                      Statement **out_stmt) {
    *out_stmt = NULL;
    if (*i >= token_count) return 0;

    if (peek(tokens, token_count, i, TOK_LET)) {
        (*i)++;
        if (*i >= token_count || tokens[*i].kind != TOK_IDENTIFIER || !tokens[*i].value) {
            set_error("expected variable name after 'let'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_LET;
        s->let_name = strdup(tokens[*i].value);
        if (!s->let_name) { free(s); return 0; }
        (*i)++;
        s->let_type = NULL;
        if (peek(tokens, token_count, i, TOK_COLON)) {
            (*i)++;
            if (*i >= token_count || tokens[*i].kind != TOK_IDENTIFIER || !tokens[*i].value) {
                free(s->let_name);
                free(s);
                set_error("expected type after ':'");
                return 0;
            }
            s->let_type = strdup(tokens[*i].value);
            if (!s->let_type) { free(s->let_name); free(s); return 0; }
            (*i)++;
            if (peek(tokens, token_count, i, TOK_LANGLE)) {
                char buf[64];
                size_t pos = (size_t)snprintf(buf, sizeof(buf), "%s<", s->let_type);
                (*i)++;
                if (*i < token_count && tokens[*i].kind == TOK_INTEGER_LITERAL && tokens[*i].value) {
                    size_t nlen = strlen(tokens[*i].value);
                    if (pos + nlen + 2 < sizeof(buf)) {
                        memcpy(buf + pos, tokens[*i].value, nlen + 1);
                        pos += nlen;
                    }
                    (*i)++;
                }
                if (*i < token_count && tokens[*i].kind == TOK_RANGLE) {
                    buf[pos++] = '>';
                    buf[pos] = '\0';
                    (*i)++;
                }
                free(s->let_type);
                s->let_type = strdup(buf);
                if (!s->let_type) { free(s->let_name); free(s); return 0; }
            }
        }
        if (!peek(tokens, token_count, i, TOK_OPERATOR) || !tokens[*i].value || strcmp(tokens[*i].value, "=") != 0) {
            free(s->let_type);
            free(s->let_name);
            free(s);
            set_error("expected '=' in let binding");
            return 0;
        }
        (*i)++;
        s->init = (Expr *)malloc(sizeof(Expr));
        if (!s->init) { free(s->let_type); free(s->let_name); free(s); return 0; }
        if (!parse_expr(tokens, token_count, i, s->init)) {
            ast_free_expr(s->init);
            free(s->init);
            free(s->let_type);
            free(s->let_name);
            free(s);
            return 0;
        }
        if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
            ast_free_statement(s);
            free(s);
            set_error("expected ';' after let binding");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_QUANTUM)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_LBRACE)) {
            set_error("expected '{' after 'quantum'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_QUANTUM_BLOCK;
        s->body = NULL;
        s->body_count = 0;
        size_t cap = 0;
        while (*i < token_count && !peek(tokens, token_count, i, TOK_RBRACE)) {
            Statement *inner = NULL;
            if (!parse_stmt(tokens, token_count, i, &inner)) {
                ast_free_statement(s);
                free(s);
                return 0;
            }
            if (!inner) break;
            if (s->body_count >= cap) {
                size_t new_cap = cap ? cap * 2 : BUF_INIT;
                Statement *n = (Statement *)realloc(s->body, new_cap * sizeof(Statement));
                if (!n) {
                    ast_free_statement(inner);
                    free(inner);
                    ast_free_statement(s);
                    free(s);
                    return 0;
                }
                s->body = n;
                cap = new_cap;
            }
            s->body[s->body_count++] = *inner;
            free(inner);
        }
        if (!consume(tokens, token_count, i, TOK_RBRACE)) {
            ast_free_statement(s);
            free(s);
            set_error("expected '}' to close quantum block");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    /* Expression statement: expr ; */
    Expr ex;
    if (!parse_expr(tokens, token_count, i, &ex)) return 0;
    if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
        ast_free_expr(&ex);
        set_error("expected ';' after expression");
        return 0;
    }
    Statement *s = (Statement *)calloc(1, sizeof(Statement));
    if (!s) { ast_free_expr(&ex); return 0; }
    s->kind = STMT_EXPR;
    s->init = (Expr *)malloc(sizeof(Expr));
    if (!s->init) { ast_free_expr(&ex); free(s); return 0; }
    *s->init = ex;
    *out_stmt = s;
    return 1;
}

/* Parse function body: statements until RBRACE. */
static int parse_body(const Token *tokens, size_t token_count, size_t *i,
                      Statement **out_body, size_t *out_count) {
    Statement *body = NULL;
    size_t count = 0, cap = 0;

    while (*i < token_count && !peek(tokens, token_count, i, TOK_RBRACE)) {
        Statement *s = NULL;
        if (!parse_stmt(tokens, token_count, i, &s)) {
            if (body) {
                for (size_t k = 0; k < count; k++) ast_free_statement(&body[k]);
                free(body);
            }
            return 0;
        }
        if (!s) break;
        if (count >= cap) {
            size_t new_cap = cap ? cap * 2 : BUF_INIT;
            Statement *n = (Statement *)realloc(body, new_cap * sizeof(Statement));
            if (!n) {
                ast_free_statement(s);
                free(s);
                if (body) {
                    for (size_t k = 0; k < count; k++) ast_free_statement(&body[k]);
                    free(body);
                }
                return 0;
            }
            body = n;
            cap = new_cap;
        }
        body[count++] = *s;
        free(s);
    }
    if (peek(tokens, token_count, i, TOK_RBRACE)) (*i)++;
    *out_body = body;
    *out_count = count;
    return 1;
}

Program *parse(const Token *tokens, size_t token_count) {
    parse_last_error[0] = '\0';
    Function *list = NULL;
    size_t count = 0, cap = 0;
    size_t i = 0;

    while (i < token_count) {
        while (i < token_count && tokens[i].kind != TOK_FN)
            i++;
        if (i >= token_count) break;
        i++;
        if (i >= token_count || tokens[i].kind != TOK_IDENTIFIER || !tokens[i].value) {
            set_error("expected function name after 'fn'");
            goto fail;
        }
        char *name = strdup(tokens[i].value);
        if (!name) { set_error("out of memory"); goto fail; }
        i++;

        if (!consume(tokens, token_count, &i, TOK_LPAREN)) {
            free(name);
            set_error("expected '(' after function name");
            goto fail;
        }
        if (!consume(tokens, token_count, &i, TOK_RPAREN)) {
            free(name);
            set_error("expected ')' (no parameters yet)");
            goto fail;
        }
        if (!consume(tokens, token_count, &i, TOK_ARROW)) {
            free(name);
            set_error("expected '->' return type");
            goto fail;
        }
        if (i >= token_count || tokens[i].kind != TOK_IDENTIFIER) {
            free(name);
            set_error("expected return type after '->'");
            goto fail;
        }
        i++;
        if (i < token_count && tokens[i].kind == TOK_LANGLE) {
            do {
                if (tokens[i].kind == TOK_LANGLE) i++;
                else if (tokens[i].kind == TOK_RANGLE) { i++; break; }
                i++;
            } while (i < token_count);
        }

        if (!peek(tokens, token_count, &i, TOK_LBRACE)) {
            free(name);
            set_error("expected '{' before function body");
            goto fail;
        }
        i++;
        Statement *body = NULL;
        size_t body_count = 0;
        if (!parse_body(tokens, token_count, &i, &body, &body_count)) {
            free(name);
            set_error("failed to parse function body");
            goto fail;
        }

        if (count >= cap) {
            size_t new_cap = cap ? cap * 2 : BUF_INIT;
            Function *n = (Function *)realloc(list, new_cap * sizeof(Function));
            if (!n) {
                free(name);
                if (body) {
                    for (size_t k = 0; k < body_count; k++) ast_free_statement(&body[k]);
                    free(body);
                }
                set_error("out of memory");
                goto fail;
            }
            list = n;
            cap = new_cap;
        }
        list[count].name = name;
        list[count].body = body;
        list[count].body_count = body_count;
        count++;
    }

    Program *p = (Program *)malloc(sizeof(Program));
    if (!p) { set_error("out of memory"); goto fail; }
    p->functions = list;
    p->function_count = count;
    return p;

fail:
    if (list) {
        for (size_t j = 0; j < count; j++) {
            free(list[j].name);
            if (list[j].body) {
                for (size_t k = 0; k < list[j].body_count; k++) ast_free_statement(&list[j].body[k]);
                free(list[j].body);
            }
        }
        free(list);
    }
    return NULL;
}
