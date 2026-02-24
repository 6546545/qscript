#ifndef QSCRIPT_AST_H
#define QSCRIPT_AST_H

#include <stddef.h>

typedef struct Function {
    char *name; /* owned */
} Function;

typedef struct Program {
    Function *functions;
    size_t function_count;
} Program;

void ast_free(Program *program);

#endif /* QSCRIPT_AST_H */
