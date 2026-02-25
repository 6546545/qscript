#include "typecheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERR_MAX 256
static char typecheck_error[ERR_MAX];
static const Program *typecheck_program;

static int is_function_name(const char *name) {
    if (!typecheck_program || !name) return 0;
    for (size_t i = 0; i < typecheck_program->function_count; i++) {
        if (strcmp(typecheck_program->functions[i].name, name) == 0) return 1;
    }
    return 0;
}

const char *typecheck_get_last_error(void) {
    return typecheck_error;
}

/* Simple scope: map variable names to type strings. MVP uses a fixed-size table. */
#define SCOPE_MAX 32
typedef struct {
    char *name;
    char *type;
} ScopeEntry;
static ScopeEntry scope[SCOPE_MAX];
static size_t scope_count;

static void scope_push(const char *name, const char *type) {
    if (scope_count >= SCOPE_MAX) return;
    scope[scope_count].name = name ? strdup(name) : NULL;
    scope[scope_count].type = type ? strdup(type) : NULL;
    scope_count++;
}

static void scope_pop(void) {
    if (scope_count == 0) return;
    scope_count--;
    free(scope[scope_count].name);
    free(scope[scope_count].type);
}

static const char *scope_lookup(const char *name) {
    for (size_t i = scope_count; i > 0; i--) {
        if (scope[i - 1].name && strcmp(scope[i - 1].name, name) == 0)
            return scope[i - 1].type;
    }
    return NULL;
}

static void scope_clear(void) {
    while (scope_count > 0) scope_pop();
}

static int check_expr(const Expr *e);

static int check_expr(const Expr *e) {
    if (!e) return 0;
    switch (e->kind) {
        case EXPR_STR:
            return 0;  /* string type */
        case EXPR_INT:
            return 0;  /* i32 */
        case EXPR_IDENT:
            if (e->value && !scope_lookup(e->value)) {
                if (strcmp(e->value, "print") == 0 || strcmp(e->value, "print_bits") == 0 ||
                    strcmp(e->value, "alloc_qreg<2>") == 0 || strcmp(e->value, "h") == 0 ||
                    strcmp(e->value, "cx") == 0 || strcmp(e->value, "measure_all") == 0 ||
                    strcmp(e->value, "result") == 0)
                    return 0;
                if (is_function_name(e->value)) return 0;
                (void)snprintf(typecheck_error, sizeof(typecheck_error),
                    "undefined variable '%s'", e->value);
                return -1;
            }
            return 0;
        case EXPR_INDEX:
            if (check_expr(e->base) != 0 || check_expr(e->index) != 0) return -1;
            return 0;
        case EXPR_BINARY:
            if (check_expr(e->base) != 0 || check_expr(e->right) != 0) return -1;
            return 0;
        case EXPR_CALL:
            if (!e->base) return -1;
            if (check_expr(e->base) != 0) return -1;
            for (size_t i = 0; i < e->arg_count; i++) {
                if (check_expr(&e->args[i]) != 0) return -1;
            }
            return 0;
    }
    return 0;
}

static int check_statement(const Statement *s);

static int check_statement(const Statement *s) {
    if (!s) return 0;
    switch (s->kind) {
        case STMT_CALL:
            if (s->callee && !scope_lookup(s->callee)) {
                if (strcmp(s->callee, "print") != 0 && strcmp(s->callee, "print_bits") != 0) {
                    (void)snprintf(typecheck_error, sizeof(typecheck_error),
                        "undefined function '%s'", s->callee);
                    return -1;
                }
            }
            for (size_t i = 0; i < s->arg_count; i++) {
                if (check_expr(&s->args[i]) != 0) return -1;
            }
            return 0;
        case STMT_LET:
            if (s->let_name) scope_push(s->let_name, s->let_type);
            if (s->init && check_expr(s->init) != 0) return -1;
            return 0;
        case STMT_QUANTUM_BLOCK:
            for (size_t i = 0; i < s->body_count; i++) {
                if (check_statement(&s->body[i]) != 0) return -1;
            }
            return 0;
        case STMT_IF:
            if (s->init && check_expr(s->init) != 0) return -1;
            for (size_t i = 0; i < s->body_count; i++) {
                if (check_statement(&s->body[i]) != 0) return -1;
            }
            for (size_t i = 0; i < s->else_body_count; i++) {
                if (check_statement(&s->else_body[i]) != 0) return -1;
            }
            return 0;
        case STMT_EXPR:
            if (s->init && check_expr(s->init) != 0) return -1;
            return 0;
    }
    return 0;
}

int typecheck(const Program *program) {
    typecheck_error[0] = '\0';
    scope_clear();
    typecheck_program = program;

    if (!program || program->function_count == 0) {
        (void)snprintf(typecheck_error, sizeof(typecheck_error), "empty program");
        return -1;
    }

    int has_main = 0;
    for (size_t i = 0; i < program->function_count; i++) {
        const Function *f = &program->functions[i];
        if (strcmp(f->name, "main") == 0) {
            has_main = 1;
            break;
        }
    }
    if (!has_main) {
        (void)snprintf(typecheck_error, sizeof(typecheck_error),
            "program must define main() -> unit");
        return -1;
    }

    for (size_t i = 0; i < program->function_count; i++) {
        const Function *f = &program->functions[i];
        if (strcmp(f->name, "main") == 0 && f->param_count != 0) {
            (void)snprintf(typecheck_error, sizeof(typecheck_error),
                "main() must have no parameters");
            return -1;
        }
        scope_clear();
        for (size_t j = 0; j < f->param_count; j++) {
            scope_push(f->params[j].name, f->params[j].type);
        }
        for (size_t j = 0; j < f->body_count; j++) {
            if (check_statement(&f->body[j]) != 0) return -1;
        }
    }
    scope_clear();
    return 0;
}
