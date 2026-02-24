#ifndef QSCRIPT_IR_H
#define QSCRIPT_IR_H

#include "ast.h"
#include <stddef.h>

typedef struct Instruction {
    enum { IR_NOP } kind;
} Instruction;

typedef struct BasicBlock {
    char *name; /* owned */
    Instruction *instructions;
    size_t instruction_count;
} BasicBlock;

typedef struct FunctionIr {
    char *name; /* owned */
    BasicBlock *blocks;
    size_t block_count;
} FunctionIr;

typedef struct ModuleIr {
    FunctionIr *functions;
    size_t function_count;
} ModuleIr;

/**
 * Lower AST to IR. Returns newly allocated ModuleIr (caller must ir_free).
 */
ModuleIr *lower_to_ir(const Program *program);

void ir_free(ModuleIr *module);

#endif /* QSCRIPT_IR_H */
