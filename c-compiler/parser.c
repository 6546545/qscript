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

/* Advance index past optional whitespace / EOF; return 1 if more tokens. */
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

/* Consume balanced { } and return 1. Advances *i past the closing }. */
static int skip_balanced_braces(const Token *tokens, size_t token_count, size_t *i) {
    if (*i >= token_count || tokens[*i].kind != TOK_LBRACE) return 0;
    size_t depth = 1;
    (*i)++;
    while (*i < token_count && depth > 0) {
        if (tokens[*i].kind == TOK_LBRACE) depth++;
        else if (tokens[*i].kind == TOK_RBRACE) depth--;
        (*i)++;
    }
    return depth == 0;
}

Program *parse(const Token *tokens, size_t token_count) {
    parse_last_error[0] = '\0';
    Function *list = NULL;
    size_t count = 0, cap = 0;
    size_t i = 0;

    while (i < token_count) {
        if (tokens[i].kind == TOK_EOF) break;
        if (tokens[i].kind != TOK_FN) {
            set_error("expected 'fn' or end of file");
            goto fail;
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
        /* Return type: identifier (unit, i32) or generic (e.g. qreg<2>) */
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
        if (!skip_balanced_braces(tokens, token_count, &i)) {
            free(name);
            set_error("unclosed '{' in function body");
            goto fail;
        }

        if (count >= cap) {
            size_t new_cap = cap ? cap * 2 : BUF_INIT;
            Function *n = (Function *)realloc(list, new_cap * sizeof(Function));
            if (!n) { free(name); set_error("out of memory"); goto fail; }
            list = n;
            cap = new_cap;
        }
        list[count].name = name;
        count++;
    }

    Program *p = (Program *)malloc(sizeof(Program));
    if (!p) { set_error("out of memory"); goto fail; }
    p->functions = list;
    p->function_count = count;
    return p;

fail:
    if (list) {
        for (size_t j = 0; j < count; j++) free(list[j].name);
        free(list);
    }
    return NULL;
}
