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
    } else if (e->kind == EXPR_BINARY) {
        ast_free_expr(e->base);
        ast_free_expr(e->right);
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
        case STMT_ASSIGN:
            free(s->let_name);
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
        case STMT_IF:
            if (s->init) ast_free_expr(s->init);
            if (s->body) {
                for (size_t i = 0; i < s->body_count; i++) ast_free_statement(&s->body[i]);
                free(s->body);
            }
            if (s->else_body) {
                for (size_t i = 0; i < s->else_body_count; i++) ast_free_statement(&s->else_body[i]);
                free(s->else_body);
            }
            break;
        case STMT_RETURN:
            if (s->init) ast_free_expr(s->init);
            break;
        case STMT_LOOP:
            if (s->body) {
                for (size_t i = 0; i < s->body_count; i++) ast_free_statement(&s->body[i]);
                free(s->body);
            }
            break;
        case STMT_FOR:
            if (s->for_init) { ast_free_statement(s->for_init); free(s->for_init); }
            if (s->for_step) { ast_free_statement(s->for_step); free(s->for_step); }
            if (s->init) ast_free_expr(s->init);
            if (s->body) {
                for (size_t i = 0; i < s->body_count; i++) ast_free_statement(&s->body[i]);
                free(s->body);
            }
            break;
        case STMT_WHILE:
            if (s->init) ast_free_expr(s->init);
            if (s->body) {
                for (size_t i = 0; i < s->body_count; i++) ast_free_statement(&s->body[i]);
                free(s->body);
            }
            break;
        case STMT_BREAK:
            break;
        case STMT_CONTINUE:
            break;
    }
}

void ast_free(Program *program) {
    if (!program) return;
    if (program->type_aliases) {
        for (size_t i = 0; i < program->type_alias_count; i++) {
            free(program->type_aliases[i].name);
            free(program->type_aliases[i].value);
        }
        free(program->type_aliases);
        program->type_aliases = NULL;
        program->type_alias_count = 0;
    }
    for (size_t i = 0; i < program->function_count; i++) {
        Function *f = &program->functions[i];
        free(f->name);
        free(f->return_type);
        if (f->params) {
            for (size_t j = 0; j < f->param_count; j++) {
                free(f->params[j].name);
                free(f->params[j].type);
            }
            free(f->params);
        }
        if (f->body) {
            for (size_t j = 0; j < f->body_count; j++) ast_free_statement(&f->body[j]);
            free(f->body);
        }
    }
    free(program->functions);
    program->functions = NULL;
    program->function_count = 0;
}
