#ifndef QSCRIPT_IR_H
#define QSCRIPT_IR_H

#include "ast.h"
#include <stddef.h>

typedef enum { IR_VAL_STR, IR_VAL_INT } IrValueKind;

typedef struct IrValue {
    IrValueKind kind;
    char *value; /* owned */
} IrValue;

typedef enum { IR_NOP, IR_CALL, IR_RET, IR_COND_BR, IR_BR } InstructionKind;

typedef struct Instruction {
    InstructionKind kind;
    char *callee;     /* owned; for IR_CALL; for IR_COND_BR: comparison op e.g. "<" */
    IrValue *args;   /* owned array; for IR_CALL; for IR_COND_BR: [left, right] */
    size_t arg_count;
    char *block_target;  /* for IR_BR: target block name; for IR_COND_BR: then block (else is block_target_else) */
    char *block_target_else;  /* for IR_COND_BR: else block name */
} Instruction;

typedef struct BasicBlock {
    char *name; /* owned */
    Instruction *instructions;
    size_t instruction_count;
} BasicBlock;

typedef struct ParamIr {
    char *name;   /* owned */
    char *type;   /* owned, e.g. "i32", "unit", "qreg<2>" */
} ParamIr;

typedef struct FunctionIr {
    char *name;       /* owned */
    ParamIr *params;  /* owned array; may be NULL if param_count == 0 */
    size_t param_count;
    char *return_type; /* owned */
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
