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

/* Parse one expression; return 1 on success, 0 on failure. On success, *e is filled and i advanced. */
static int parse_expr(const Token *tokens, size_t token_count, size_t *i, Expr *e) {
    if (*i >= token_count) return 0;
    const Token *t = &tokens[*i];
    e->value = NULL;
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
        return 1;
    }
    return 0;
}

/* Parse function body: statements until RBRACE. Body array and count updated. Return 1 on success. */
static int parse_body(const Token *tokens, size_t token_count, size_t *i,
                      Statement **out_body, size_t *out_count) {
    Statement *body = NULL;
    size_t count = 0, cap = 0;

    while (*i < token_count && tokens[*i].kind != TOK_RBRACE) {
        if (tokens[*i].kind == TOK_IDENTIFIER && tokens[*i].value) {
            char *callee = strdup(tokens[*i].value);
            if (!callee) goto fail_body;
            size_t j = *i + 1;
            if (j >= token_count || tokens[j].kind != TOK_LPAREN) {
                free(callee);
                goto skip_to_semi;
            }
            j++; /* past ( */
            Expr *args = NULL;
            size_t arg_count = 0, arg_cap = 0;
            while (j < token_count && tokens[j].kind != TOK_RPAREN) {
                Expr ex;
                if (!parse_expr(tokens, token_count, &j, &ex)) {
                    if (args) {
                        for (size_t k = 0; k < arg_count; k++) free(args[k].value);
                        free(args);
                    }
                    free(callee);
                    goto skip_to_semi;
                }
                if (arg_count >= arg_cap) {
                    size_t new_cap = arg_cap ? arg_cap * 2 : 4;
                    Expr *n = (Expr *)realloc(args, new_cap * sizeof(Expr));
                    if (!n) {
                        free(ex.value);
                        if (args) {
                            for (size_t k = 0; k < arg_count; k++) free(args[k].value);
                            free(args);
                        }
                        free(callee);
                        goto skip_to_semi;
                    }
                    args = n;
                    arg_cap = new_cap;
                }
                args[arg_count++] = ex;
                if (j < token_count && tokens[j].kind == TOK_COMMA) j++;
            }
            if (j >= token_count || tokens[j].kind != TOK_RPAREN) {
                if (args) {
                    for (size_t k = 0; k < arg_count; k++) free(args[k].value);
                    free(args);
                }
                free(callee);
                goto skip_to_semi;
            }
            j++;
            if (j >= token_count || tokens[j].kind != TOK_SEMICOLON) {
                if (args) {
                    for (size_t k = 0; k < arg_count; k++) free(args[k].value);
                    free(args);
                }
                free(callee);
                goto skip_to_semi;
            }
            j++;
            *i = j;

            if (count >= cap) {
                size_t new_cap = cap ? cap * 2 : BUF_INIT;
                Statement *n = (Statement *)realloc(body, new_cap * sizeof(Statement));
                if (!n) {
                    if (args) { for (size_t k = 0; k < arg_count; k++) free(args[k].value); free(args); }
                    free(callee);
                    goto fail_body;
                }
                body = n;
                cap = new_cap;
            }
            body[count].kind = STMT_CALL;
            body[count].callee = callee;
            body[count].args = args;
            body[count].arg_count = arg_count;
            count++;
            continue;
        }
skip_to_semi:
        while (*i < token_count && tokens[*i].kind != TOK_SEMICOLON && tokens[*i].kind != TOK_RBRACE)
            (*i)++;
        if (*i < token_count && tokens[*i].kind == TOK_SEMICOLON) (*i)++;
    }
    if (*i < token_count && tokens[*i].kind == TOK_RBRACE) (*i)++;
    *out_body = body;
    *out_count = count;
    return 1;

fail_body:
    if (body) {
        for (size_t k = 0; k < count; k++) {
            free(body[k].callee);
            if (body[k].args) {
                for (size_t a = 0; a < body[k].arg_count; a++) free(body[k].args[a].value);
                free(body[k].args);
            }
        }
        free(body);
    }
    return 0;
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
            size_t depth = 0;
            do {
                if (tokens[i].kind == TOK_LANGLE) depth++;
                else if (tokens[i].kind == TOK_RANGLE) { depth--; if (depth == 0) { i++; break; } }
                i++;
            } while (i < token_count);
        }

        if (!peek(tokens, token_count, &i, TOK_LBRACE)) {
            free(name);
            set_error("expected '{' before function body");
            goto fail;
        }
        i++; /* consume { */
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
                    for (size_t k = 0; k < body_count; k++) {
                        free(body[k].callee);
                        if (body[k].args) {
                            for (size_t a = 0; a < body[k].arg_count; a++) free(body[k].args[a].value);
                            free(body[k].args);
                        }
                    }
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
                for (size_t k = 0; k < list[j].body_count; k++) {
                    free(list[j].body[k].callee);
                    if (list[j].body[k].args) {
                        for (size_t a = 0; a < list[j].body[k].arg_count; a++)
                            free(list[j].body[k].args[a].value);
                        free(list[j].body[k].args);
                    }
                }
                free(list[j].body);
            }
        }
        free(list);
    }
    return NULL;
}
