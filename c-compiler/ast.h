#ifndef QSCRIPT_AST_H
#define QSCRIPT_AST_H

#include <stddef.h>

typedef enum { EXPR_STR, EXPR_INT, EXPR_IDENT, EXPR_CALL, EXPR_INDEX } ExprKind;

typedef struct Expr Expr;

typedef struct Expr {
    ExprKind kind;
    char *value;       /* owned; for STR, INT, IDENT */
    Expr *base;        /* for EXPR_INDEX: base[index]; for EXPR_CALL: callee (as IDENT) */
    Expr *index;       /* for EXPR_INDEX only */
    Expr *args;        /* owned array; for EXPR_CALL only */
    size_t arg_count;
} Expr;

typedef enum { STMT_CALL, STMT_LET, STMT_QUANTUM_BLOCK, STMT_EXPR } StatementKind;

typedef struct Statement Statement;

typedef struct Statement {
    StatementKind kind;
    char *callee;      /* owned; for STMT_CALL */
    char *let_name;    /* owned; for STMT_LET */
    char *let_type;    /* owned; optional type annotation for STMT_LET */
    Expr *init;        /* for STMT_LET: init expr; for STMT_CALL/STMT_EXPR: args or expr */
    Expr *args;        /* owned array; for STMT_CALL */
    size_t arg_count;
    Statement *body;   /* owned array; for STMT_QUANTUM_BLOCK */
    size_t body_count;
} Statement;

typedef struct Function {
    char *name;           /* owned */
    Statement *body;      /* owned array */
    size_t body_count;
} Function;

typedef struct Program {
    Function *functions;
    size_t function_count;
} Program;

void ast_free(Program *program);
void ast_free_expr(Expr *e);
void ast_free_statement(Statement *s);

#endif /* QSCRIPT_AST_H */
