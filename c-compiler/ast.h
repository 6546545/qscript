#ifndef QSCRIPT_AST_H
#define QSCRIPT_AST_H

#include <stddef.h>

typedef enum { EXPR_STR, EXPR_INT, EXPR_BOOL, EXPR_IDENT, EXPR_CALL, EXPR_INDEX, EXPR_BINARY } ExprKind;

typedef struct Expr Expr;

typedef struct Expr {
    ExprKind kind;
    char *value;       /* owned; for STR, INT, IDENT; for EXPR_BINARY: operator e.g. "<", "==" */
    Expr *base;        /* for EXPR_INDEX: base[index]; for EXPR_CALL: callee; for EXPR_BINARY: left */
    Expr *index;       /* for EXPR_INDEX only */
    Expr *right;       /* for EXPR_BINARY: right operand */
    Expr *args;        /* owned array; for EXPR_CALL only */
    size_t arg_count;
} Expr;

typedef enum { STMT_CALL, STMT_LET, STMT_ASSIGN, STMT_QUANTUM_BLOCK, STMT_EXPR, STMT_IF, STMT_RETURN, STMT_LOOP, STMT_FOR, STMT_WHILE, STMT_BREAK, STMT_CONTINUE } StatementKind;

typedef struct Statement Statement;

typedef struct Statement {
    StatementKind kind;
    char *callee;      /* owned; for STMT_CALL */
    char *let_name;    /* owned; for STMT_LET */
    char *let_type;    /* owned; optional type annotation for STMT_LET */
    Expr *init;        /* for STMT_LET: init expr; for STMT_CALL/STMT_EXPR: args or expr; for STMT_IF: condition; for STMT_RETURN: optional value */
    Expr *args;        /* owned array; for STMT_CALL */
    size_t arg_count;
    Statement *body;   /* owned array; for STMT_QUANTUM_BLOCK, STMT_IF then-branch */
    size_t body_count;
    Statement *else_body;  /* owned array; for STMT_IF else-branch; NULL if no else */
    size_t else_body_count;
    Statement *for_init;   /* for STMT_FOR: init stmt (e.g. let i=0); owned, may be NULL */
    Statement *for_step;   /* for STMT_FOR: step stmt (e.g. i=i+1); owned, may be NULL */
} Statement;

typedef struct Param {
    char *name;   /* owned */
    char *type;   /* owned, e.g. "i32", "unit", "qreg<2>" */
} Param;

typedef struct Function {
    char *name;           /* owned */
    Param *params;        /* owned array; may be NULL if param_count == 0 */
    size_t param_count;
    char *return_type;    /* owned, e.g. "unit", "i32" */
    Statement *body;      /* owned array */
    size_t body_count;
} Function;

typedef struct TypeAlias {
    char *name;   /* owned */
    char *value;  /* owned, e.g. "i32", "qreg<2>" */
} TypeAlias;

typedef struct Program {
    TypeAlias *type_aliases;
    size_t type_alias_count;
    Function *functions;
    size_t function_count;
} Program;

void ast_free(Program *program);
void ast_free_expr(Expr *e);
void ast_free_statement(Statement *s);

#endif /* QSCRIPT_AST_H */
