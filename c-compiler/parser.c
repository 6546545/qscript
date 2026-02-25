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
static int parse_expr_unary(const Token *tokens, size_t token_count, size_t *i, Expr *e);
static int parse_expr(const Token *tokens, size_t token_count, size_t *i, Expr *e);
static int parse_stmt(const Token *tokens, size_t token_count, size_t *i,
                      Statement **out_stmt);

static int is_binary_op(const Token *tokens, size_t token_count, size_t i) {
    if (i >= token_count) return 0;
    const Token *t = &tokens[i];
    if (t->kind == TOK_LANGLE || t->kind == TOK_RANGLE) return 1;
    if (t->kind == TOK_OPERATOR && t->value) {
        if (strcmp(t->value, "==") == 0 || strcmp(t->value, "!=") == 0 ||
            strcmp(t->value, "<=") == 0 || strcmp(t->value, ">=") == 0 ||
            strcmp(t->value, "+") == 0 || strcmp(t->value, "-") == 0 ||
            strcmp(t->value, "*") == 0 || strcmp(t->value, "/") == 0 || strcmp(t->value, "%") == 0 ||
            strcmp(t->value, "&&") == 0 || strcmp(t->value, "||") == 0)
            return 1;
    }
    return 0;
}

static const char *binary_op_value(const Token *tokens, size_t i) {
    const Token *t = &tokens[i];
    if (t->kind == TOK_LANGLE) return "<";
    if (t->kind == TOK_RANGLE) return ">";
    return t->value ? t->value : "?";
}

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
    if (t->kind == TOK_TRUE) {
        e->kind = EXPR_BOOL;
        e->value = strdup("1");
        if (!e->value) return 0;
        (*i)++;
        return 1;
    }
    if (t->kind == TOK_FALSE) {
        e->kind = EXPR_BOOL;
        e->value = strdup("0");
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

/* Parse expression: expr_unary (op expr_unary)* for binary operators */
static int parse_expr(const Token *tokens, size_t token_count, size_t *i, Expr *e) {
    if (!parse_expr_unary(tokens, token_count, i, e)) return 0;
    while (is_binary_op(tokens, token_count, *i)) {
        const char *op = binary_op_value(tokens, *i);
        (*i)++;
        Expr right;
        if (!parse_expr_unary(tokens, token_count, i, &right)) {
            ast_free_expr(e);
            return 0;
        }
        Expr *left = (Expr *)malloc(sizeof(Expr));
        if (!left) { ast_free_expr(e); ast_free_expr(&right); return 0; }
        *left = *e;
        memset(e, 0, sizeof(Expr));
        e->kind = EXPR_BINARY;
        e->value = strdup(op);
        if (!e->value) { ast_free_expr(left); ast_free_expr(&right); free(left); return 0; }
        e->base = left;
        e->right = (Expr *)malloc(sizeof(Expr));
        if (!e->right) { ast_free_expr(left); ast_free_expr(&right); free(e->value); return 0; }
        *e->right = right;
    }
    return 1;
}

/* Parse expression unary: "-" unary | "!" unary | primary postfix* where postfix = [ expr ] | ( args ) */
static int parse_expr_unary(const Token *tokens, size_t token_count, size_t *i, Expr *e) {
    if (*i < token_count && tokens[*i].kind == TOK_OPERATOR && tokens[*i].value &&
        strcmp(tokens[*i].value, "-") == 0) {
        (*i)++;
        Expr inner;
        if (!parse_expr_unary(tokens, token_count, i, &inner)) return 0;
        Expr *base = (Expr *)malloc(sizeof(Expr));
        if (!base) { ast_free_expr(&inner); return 0; }
        memset(base, 0, sizeof(Expr));
        base->kind = EXPR_INT;
        base->value = strdup("0");
        if (!base->value) { ast_free_expr(&inner); free(base); return 0; }
        Expr *right = (Expr *)malloc(sizeof(Expr));
        if (!right) { ast_free_expr(&inner); free(base->value); free(base); return 0; }
        *right = inner;
        memset(e, 0, sizeof(Expr));
        e->kind = EXPR_BINARY;
        e->value = strdup("-");
        e->base = base;
        e->right = right;
        return e->value ? 1 : 0;
    }
    if (*i < token_count && tokens[*i].kind == TOK_OPERATOR && tokens[*i].value &&
        strcmp(tokens[*i].value, "!") == 0) {
        (*i)++;
        Expr inner;
        if (!parse_expr_unary(tokens, token_count, i, &inner)) return 0;
        Expr *base = (Expr *)malloc(sizeof(Expr));
        if (!base) { ast_free_expr(&inner); return 0; }
        *base = inner;
        memset(e, 0, sizeof(Expr));
        e->kind = EXPR_BINARY;
        e->value = strdup("!");
        e->base = base;
        e->right = NULL;  /* unary not */
        return e->value ? 1 : 0;
    }
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

    if (peek(tokens, token_count, i, TOK_IF)) {
        (*i)++;
        Expr cond;
        if (!parse_expr(tokens, token_count, i, &cond)) {
            set_error("expected condition after 'if'");
            return 0;
        }
        if (!consume(tokens, token_count, i, TOK_LBRACE)) {
            ast_free_expr(&cond);
            set_error("expected '{' after if condition");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) { ast_free_expr(&cond); return 0; }
        s->kind = STMT_IF;
        s->init = (Expr *)malloc(sizeof(Expr));
        if (!s->init) { ast_free_expr(&cond); free(s); return 0; }
        *s->init = cond;
        s->body = NULL;
        s->body_count = 0;
        s->else_body = NULL;
        s->else_body_count = 0;
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
            set_error("expected '}' to close if body");
            return 0;
        }
        if (peek(tokens, token_count, i, TOK_ELSE)) {
            (*i)++;
            if (peek(tokens, token_count, i, TOK_IF)) {
                /* else if (cond) { body } - parse as single if, put in else_body */
                Statement *elif = NULL;
                if (!parse_stmt(tokens, token_count, i, &elif) || !elif) {
                    ast_free_statement(s);
                    free(s);
                    return 0;
                }
                s->else_body = (Statement *)malloc(sizeof(Statement));
                if (!s->else_body) {
                    ast_free_statement(elif);
                    free(elif);
                    ast_free_statement(s);
                    free(s);
                    return 0;
                }
                s->else_body[0] = *elif;
                s->else_body_count = 1;
                free(elif);
            } else {
                if (!consume(tokens, token_count, i, TOK_LBRACE)) {
                    ast_free_statement(s);
                    free(s);
                    set_error("expected '{' after 'else'");
                    return 0;
                }
                size_t else_cap = 0;
                while (*i < token_count && !peek(tokens, token_count, i, TOK_RBRACE)) {
                Statement *inner = NULL;
                if (!parse_stmt(tokens, token_count, i, &inner)) {
                    ast_free_statement(s);
                    free(s);
                    return 0;
                }
                if (!inner) break;
                if (s->else_body_count >= else_cap) {
                    size_t new_cap = else_cap ? else_cap * 2 : BUF_INIT;
                    Statement *n = (Statement *)realloc(s->else_body, new_cap * sizeof(Statement));
                    if (!n) {
                        ast_free_statement(inner);
                        free(inner);
                        ast_free_statement(s);
                        free(s);
                        return 0;
                    }
                    s->else_body = n;
                    else_cap = new_cap;
                }
                s->else_body[s->else_body_count++] = *inner;
                free(inner);
            }
                if (!consume(tokens, token_count, i, TOK_RBRACE)) {
                    ast_free_statement(s);
                    free(s);
                    set_error("expected '}' to close else body");
                    return 0;
                }
            }
        }
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_FOR)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_LPAREN)) {
            set_error("expected '(' after 'for'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_FOR;
        s->for_init = NULL;
        s->for_step = NULL;
        s->init = NULL;
        s->body = NULL;
        s->body_count = 0;
        if (peek(tokens, token_count, i, TOK_SEMICOLON)) {
            (void)consume(tokens, token_count, i, TOK_SEMICOLON);  /* empty init */
        } else {
            Statement *init_stmt = NULL;
            if (parse_stmt(tokens, token_count, i, &init_stmt) && init_stmt) {
                s->for_init = init_stmt;
            }
        }
        /* parse condition */
        if (!peek(tokens, token_count, i, TOK_SEMICOLON)) {
            s->init = (Expr *)malloc(sizeof(Expr));
            if (s->init && !parse_expr(tokens, token_count, i, s->init)) {
                ast_free_expr(s->init);
                free(s->init);
                s->init = NULL;
            }
        }
        if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
            ast_free_statement(s);
            free(s);
            set_error("expected ';' after for condition");
            return 0;
        }
        if (!peek(tokens, token_count, i, TOK_RPAREN)) {
            /* Step: ident = expr (no semicolon) */
            if (peek(tokens, token_count, i, TOK_IDENTIFIER) && tokens[*i].value) {
                size_t next = *i + 1;
                if (next < token_count && tokens[next].kind == TOK_OPERATOR && tokens[next].value &&
                    strcmp(tokens[next].value, "=") == 0) {
                    Statement *step_stmt = (Statement *)calloc(1, sizeof(Statement));
                    if (step_stmt) {
                        step_stmt->kind = STMT_ASSIGN;
                        step_stmt->let_name = strdup(tokens[*i].value);
                        *i = next + 1;
                        step_stmt->init = (Expr *)malloc(sizeof(Expr));
                        if (step_stmt->let_name && step_stmt->init && parse_expr(tokens, token_count, i, step_stmt->init)) {
                            s->for_step = step_stmt;
                        } else {
                            ast_free_statement(step_stmt);
                            free(step_stmt);
                        }
                    }
                }
            }
        }
        if (!consume(tokens, token_count, i, TOK_RPAREN)) {
            ast_free_statement(s);
            free(s);
            set_error("expected ')' after for step");
            return 0;
        }
        if (!consume(tokens, token_count, i, TOK_LBRACE)) {
            ast_free_statement(s);
            free(s);
            set_error("expected '{' before for body");
            return 0;
        }
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
            set_error("expected '}' to close for body");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_WHILE)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_LPAREN)) {
            set_error("expected '(' after 'while'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_WHILE;
        s->init = (Expr *)malloc(sizeof(Expr));
        if (!s->init) { free(s); return 0; }
        if (!parse_expr(tokens, token_count, i, s->init)) {
            ast_free_expr(s->init);
            free(s->init);
            free(s);
            set_error("expected condition after 'while'");
            return 0;
        }
        if (!consume(tokens, token_count, i, TOK_RPAREN)) {
            ast_free_statement(s);
            free(s);
            set_error("expected ')' after while condition");
            return 0;
        }
        if (!consume(tokens, token_count, i, TOK_LBRACE)) {
            ast_free_statement(s);
            free(s);
            set_error("expected '{' before while body");
            return 0;
        }
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
            set_error("expected '}' to close while body");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_LOOP)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_LBRACE)) {
            set_error("expected '{' after 'loop'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_LOOP;
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
            set_error("expected '}' to close loop body");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_BREAK)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
            set_error("expected ';' after 'break'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_BREAK;
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_CONTINUE)) {
        (*i)++;
        if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
            set_error("expected ';' after 'continue'");
            return 0;
        }
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_CONTINUE;
        *out_stmt = s;
        return 1;
    }

    if (peek(tokens, token_count, i, TOK_RETURN)) {
        (*i)++;
        Statement *s = (Statement *)calloc(1, sizeof(Statement));
        if (!s) return 0;
        s->kind = STMT_RETURN;
        s->init = NULL;
        if (!peek(tokens, token_count, i, TOK_SEMICOLON)) {
            s->init = (Expr *)malloc(sizeof(Expr));
            if (!s->init) { free(s); return 0; }
            if (!parse_expr(tokens, token_count, i, s->init)) {
                ast_free_expr(s->init);
                free(s->init);
                free(s);
                set_error("expected expression or ';' after 'return'");
                return 0;
            }
        }
        if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
            if (s->init) ast_free_expr(s->init);
            free(s);
            set_error("expected ';' after return");
            return 0;
        }
        *out_stmt = s;
        return 1;
    }

    /* Assignment: ident = expr ; */
    if (peek(tokens, token_count, i, TOK_IDENTIFIER) && tokens[*i].value) {
        size_t next = *i + 1;
        if (next < token_count && tokens[next].kind == TOK_OPERATOR && tokens[next].value &&
            strcmp(tokens[next].value, "=") == 0) {
            Statement *s = (Statement *)calloc(1, sizeof(Statement));
            if (!s) return 0;
            s->kind = STMT_ASSIGN;
            s->let_name = strdup(tokens[*i].value);
            if (!s->let_name) { free(s); return 0; }
            *i = next + 1;  /* consume ident and = */
            s->init = (Expr *)malloc(sizeof(Expr));
            if (!s->init) { free(s->let_name); free(s); return 0; }
            if (!parse_expr(tokens, token_count, i, s->init)) {
                ast_free_expr(s->init);
                free(s->init);
                free(s->let_name);
                free(s);
                set_error("expected expression after '='");
                return 0;
            }
            if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
                ast_free_statement(s);
                free(s);
                set_error("expected ';' after assignment");
                return 0;
            }
            *out_stmt = s;
            return 1;
        }
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

/* Parse type alias: type name = type_expr ; */
static int parse_type_alias(const Token *tokens, size_t token_count, size_t *i,
                            TypeAlias *out) {
    if (!peek(tokens, token_count, i, TOK_TYPE)) return 0;
    (*i)++;
    if (*i >= token_count || tokens[*i].kind != TOK_IDENTIFIER || !tokens[*i].value) {
        set_error("expected type alias name after 'type'");
        return 0;
    }
    out->name = strdup(tokens[*i].value);
    if (!out->name) return 0;
    (*i)++;
    if (*i >= token_count || tokens[*i].kind != TOK_OPERATOR || !tokens[*i].value || strcmp(tokens[*i].value, "=") != 0) {
        free(out->name);
        set_error("expected '=' in type alias");
        return 0;
    }
    (*i)++;
    if (*i >= token_count || tokens[*i].kind != TOK_IDENTIFIER || !tokens[*i].value) {
        free(out->name);
        set_error("expected type after '='");
        return 0;
    }
    char buf[64];
    size_t pos = (size_t)snprintf(buf, sizeof(buf), "%s", tokens[*i].value);
    (*i)++;
    if (*i < token_count && tokens[*i].kind == TOK_LANGLE) {
        (*i)++;
        if (*i < token_count && tokens[*i].kind == TOK_INTEGER_LITERAL && tokens[*i].value) {
            size_t nlen = strlen(tokens[*i].value);
            if (pos + nlen + 3 < sizeof(buf)) {
                buf[pos++] = '<';
                memcpy(buf + pos, tokens[*i].value, nlen + 1);
                pos += nlen;
                (*i)++;
            }
        }
        if (*i < token_count && tokens[*i].kind == TOK_RANGLE) {
            buf[pos++] = '>';
            buf[pos] = '\0';
            (*i)++;
        }
    }
    out->value = strdup(buf);
    if (!out->value) { free(out->name); return 0; }
    if (!consume(tokens, token_count, i, TOK_SEMICOLON)) {
        free(out->name);
        free(out->value);
        set_error("expected ';' after type alias");
        return 0;
    }
    return 1;
}

Program *parse(const Token *tokens, size_t token_count) {
    parse_last_error[0] = '\0';
    TypeAlias *type_list = NULL;
    size_t type_count = 0, type_cap = 0;
    Function *list = NULL;
    size_t count = 0, cap = 0;
    size_t i = 0;

    while (i < token_count) {
        while (i < token_count && tokens[i].kind != TOK_FN && tokens[i].kind != TOK_TYPE)
            i++;
        if (i >= token_count) break;

        if (tokens[i].kind == TOK_TYPE) {
            TypeAlias ta;
            if (!parse_type_alias(tokens, token_count, &i, &ta)) goto fail;
            if (type_count >= type_cap) {
                size_t new_cap = type_cap ? type_cap * 2 : 4;
                TypeAlias *n = (TypeAlias *)realloc(type_list, new_cap * sizeof(TypeAlias));
                if (!n) { free(ta.name); free(ta.value); goto fail; }
                type_list = n;
                type_cap = new_cap;
            }
            type_list[type_count++] = ta;
            continue;
        }

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
        Param *params = NULL;
        size_t param_count = 0, param_cap = 0;
        while (!peek(tokens, token_count, &i, TOK_RPAREN)) {
            if (i >= token_count || tokens[i].kind != TOK_IDENTIFIER || !tokens[i].value) {
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("expected parameter name");
                goto fail;
            }
            char *pname = strdup(tokens[i].value);
            if (!pname) {
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("out of memory");
                goto fail;
            }
            i++;
            if (!consume(tokens, token_count, &i, TOK_COLON)) {
                free(pname);
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("expected ':' after parameter name");
                goto fail;
            }
            if (i >= token_count || tokens[i].kind != TOK_IDENTIFIER || !tokens[i].value) {
                free(pname);
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("expected parameter type");
                goto fail;
            }
            char *ptype = strdup(tokens[i].value);
            if (!ptype) {
                free(pname);
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("out of memory");
                goto fail;
            }
            i++;
            if (i < token_count && tokens[i].kind == TOK_LANGLE) {
                char buf[64];
                size_t pos = (size_t)snprintf(buf, sizeof(buf), "%s<", ptype);
                free(ptype);
                ptype = NULL;
                i++;
                if (i < token_count && tokens[i].kind == TOK_INTEGER_LITERAL && tokens[i].value) {
                    size_t nlen = strlen(tokens[i].value);
                    if (pos + nlen + 2 < sizeof(buf)) {
                        memcpy(buf + pos, tokens[i].value, nlen + 1);
                        pos += nlen;
                    }
                    i++;
                }
                if (i < token_count && tokens[i].kind == TOK_RANGLE) {
                    buf[pos++] = '>';
                    buf[pos] = '\0';
                    i++;
                }
                ptype = strdup(buf);
                if (!ptype) {
                    free(pname);
                    if (params) {
                        for (size_t k = 0; k < param_count; k++) {
                            free(params[k].name);
                            free(params[k].type);
                        }
                        free(params);
                    }
                    free(name);
                    set_error("out of memory");
                    goto fail;
                }
            }
            if (param_count >= param_cap) {
                size_t new_cap = param_cap ? param_cap * 2 : 4;
                Param *n = (Param *)realloc(params, new_cap * sizeof(Param));
                if (!n) {
                    free(pname);
                    free(ptype);
                    if (params) {
                        for (size_t k = 0; k < param_count; k++) {
                            free(params[k].name);
                            free(params[k].type);
                        }
                        free(params);
                    }
                    free(name);
                    set_error("out of memory");
                    goto fail;
                }
                params = n;
                param_cap = new_cap;
            }
            params[param_count].name = pname;
            params[param_count].type = ptype;
            param_count++;
            if (!consume(tokens, token_count, &i, TOK_COMMA))
                break;
        }
        if (!consume(tokens, token_count, &i, TOK_RPAREN)) {
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("expected ')' after parameter list");
            goto fail;
        }
        if (!consume(tokens, token_count, &i, TOK_ARROW)) {
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("expected '->' return type");
            goto fail;
        }
        if (i >= token_count || tokens[i].kind != TOK_IDENTIFIER) {
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("expected return type after '->'");
            goto fail;
        }
        char *return_type = strdup(tokens[i].value);
        if (!return_type) {
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("out of memory");
            goto fail;
        }
        i++;
        if (i < token_count && tokens[i].kind == TOK_LANGLE) {
            char buf[64];
            size_t pos = (size_t)snprintf(buf, sizeof(buf), "%s<", return_type);
            free(return_type);
            return_type = NULL;
            i++;
            if (i < token_count && tokens[i].kind == TOK_INTEGER_LITERAL && tokens[i].value) {
                size_t nlen = strlen(tokens[i].value);
                if (pos + nlen + 2 < sizeof(buf)) {
                    memcpy(buf + pos, tokens[i].value, nlen + 1);
                    pos += nlen;
                }
                i++;
            }
            if (i < token_count && tokens[i].kind == TOK_RANGLE) {
                buf[pos++] = '>';
                buf[pos] = '\0';
                i++;
            }
            return_type = strdup(buf);
            if (!return_type) {
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
                free(name);
                set_error("out of memory");
                goto fail;
            }
        }

        if (!peek(tokens, token_count, &i, TOK_LBRACE)) {
            free(return_type);
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("expected '{' before function body");
            goto fail;
        }
        i++;
        Statement *body = NULL;
        size_t body_count = 0;
        if (!parse_body(tokens, token_count, &i, &body, &body_count)) {
            free(return_type);
            if (params) {
                for (size_t k = 0; k < param_count; k++) {
                    free(params[k].name);
                    free(params[k].type);
                }
                free(params);
            }
            free(name);
            set_error("failed to parse function body");
            goto fail;
        }

        if (count >= cap) {
            size_t new_cap = cap ? cap * 2 : BUF_INIT;
            Function *n = (Function *)realloc(list, new_cap * sizeof(Function));
            if (!n) {
                free(return_type);
                if (params) {
                    for (size_t k = 0; k < param_count; k++) {
                        free(params[k].name);
                        free(params[k].type);
                    }
                    free(params);
                }
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
        list[count].params = params;
        list[count].param_count = param_count;
        list[count].return_type = return_type;
        list[count].body = body;
        list[count].body_count = body_count;
        count++;
    }

    Program *p = (Program *)malloc(sizeof(Program));
    if (!p) { set_error("out of memory"); goto fail; }
    p->type_aliases = type_list;
    p->type_alias_count = type_count;
    p->functions = list;
    p->function_count = count;
    return p;

fail:
    if (type_list) {
        for (size_t j = 0; j < type_count; j++) {
            free(type_list[j].name);
            free(type_list[j].value);
        }
        free(type_list);
    }
    if (list) {
        for (size_t j = 0; j < count; j++) {
            free(list[j].name);
            free(list[j].return_type);
            if (list[j].params) {
                for (size_t k = 0; k < list[j].param_count; k++) {
                    free(list[j].params[k].name);
                    free(list[j].params[k].type);
                }
                free(list[j].params);
            }
            if (list[j].body) {
                for (size_t k = 0; k < list[j].body_count; k++) ast_free_statement(&list[j].body[k]);
                free(list[j].body);
            }
        }
        free(list);
    }
    return NULL;
}
