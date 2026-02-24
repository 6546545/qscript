#include "ast.h"
#include <stdlib.h>

static void free_expr(Expr *e) {
    if (e && e->value) free(e->value);
}

static void free_statement(Statement *s) {
    if (!s) return;
    free(s->callee);
    if (s->args) {
        for (size_t i = 0; i < s->arg_count; i++) free_expr(&s->args[i]);
        free(s->args);
    }
}

void ast_free(Program *program) {
    if (!program) return;
    for (size_t i = 0; i < program->function_count; i++) {
        Function *f = &program->functions[i];
        free(f->name);
        if (f->body) {
            for (size_t j = 0; j < f->body_count; j++) free_statement(&f->body[j]);
            free(f->body);
        }
    }
    free(program->functions);
    program->functions = NULL;
    program->function_count = 0;
}
