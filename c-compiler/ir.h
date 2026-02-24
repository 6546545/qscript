#ifndef QSCRIPT_IR_H
#define QSCRIPT_IR_H

#include "ast.h"
#include <stddef.h>

typedef enum { IR_VAL_STR, IR_VAL_INT } IrValueKind;

typedef struct IrValue {
    IrValueKind kind;
    char *value; /* owned */
} IrValue;

typedef enum { IR_NOP, IR_CALL, IR_RET } InstructionKind;

typedef struct Instruction {
    InstructionKind kind;
    char *callee;     /* owned; for IR_CALL */
    IrValue *args;   /* owned array; for IR_CALL */
    size_t arg_count;
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

ModuleIr *lower_to_ir(const Program *program);
void ir_free(ModuleIr *module);

#endif /* QSCRIPT_IR_H */
