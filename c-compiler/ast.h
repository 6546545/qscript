#ifndef QSCRIPT_AST_H
#define QSCRIPT_AST_H

#include <stddef.h>

typedef enum { EXPR_STR, EXPR_INT, EXPR_IDENT } ExprKind;

typedef struct Expr {
    ExprKind kind;
    char *value; /* owned */
} Expr;

typedef enum { STMT_CALL } StatementKind;

typedef struct Statement {
    StatementKind kind;
    char *callee;   /* owned; for STMT_CALL */
    Expr *args;     /* owned array */
    size_t arg_count;
} Statement;

typedef struct Function {
    char *name;           /* owned */
    Statement *body;     /* owned array */
    size_t body_count;
} Function;

typedef struct Program {
    Function *functions;
    size_t function_count;
} Program;

void ast_free(Program *program);

#endif /* QSCRIPT_AST_H */
