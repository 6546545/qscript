#include "ast.h"
#include <stdlib.h>

void ast_free_expr(Expr *e) {
    if (!e) return;
    free(e->value);
    if (e->kind == EXPR_INDEX && e->base) {
        ast_free_expr(e->base);
        ast_free_expr(e->index);
    } else if (e->kind == EXPR_CALL && e->base) {
        ast_free_expr(e->base);
        if (e->args) {
            for (size_t i = 0; i < e->arg_count; i++) ast_free_expr(&e->args[i]);
            free(e->args);
        }
    }
}

void ast_free_statement(Statement *s) {
    if (!s) return;
    switch (s->kind) {
        case STMT_CALL:
            free(s->callee);
            if (s->args) {
                for (size_t i = 0; i < s->arg_count; i++) ast_free_expr(&s->args[i]);
                free(s->args);
            }
            break;
        case STMT_LET:
            free(s->let_name);
            free(s->let_type);
            if (s->init) ast_free_expr(s->init);
            break;
        case STMT_QUANTUM_BLOCK:
            if (s->body) {
                for (size_t i = 0; i < s->body_count; i++) ast_free_statement(&s->body[i]);
                free(s->body);
            }
            break;
        case STMT_EXPR:
            if (s->init) ast_free_expr(s->init);
            break;
    }
}

void ast_free(Program *program) {
    if (!program) return;
    for (size_t i = 0; i < program->function_count; i++) {
        Function *f = &program->functions[i];
        free(f->name);
        if (f->body) {
            for (size_t j = 0; j < f->body_count; j++) ast_free_statement(&f->body[j]);
            free(f->body);
        }
    }
    free(program->functions);
    program->functions = NULL;
    program->function_count = 0;
}
