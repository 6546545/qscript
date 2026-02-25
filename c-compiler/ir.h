#ifndef QSCRIPT_IR_H
#define QSCRIPT_IR_H

#include "ast.h"
#include <stddef.h>

typedef enum { IR_VAL_STR, IR_VAL_INT, IR_VAL_SSA } IrValueKind;  /* IR_VAL_SSA: SSA name e.g. "add.0" */

typedef struct IrValue {
    IrValueKind kind;
    char *value; /* owned */
} IrValue;

typedef enum { IR_NOP, IR_CALL, IR_RET, IR_COND_BR, IR_BR, IR_ALLOCA, IR_STORE, IR_ADD } InstructionKind;

typedef struct Instruction {
    InstructionKind kind;
    char *callee;     /* owned; for IR_CALL; for IR_COND_BR: comparison op; for IR_ADD: "+" etc; for IR_ALLOCA/IR_STORE: var name */
    IrValue *args;   /* owned array; for IR_CALL; for IR_COND_BR/IR_ADD: [left, right]; for IR_STORE: [value] */
    size_t arg_count;
    char *block_target;  /* for IR_BR: target block name; for IR_COND_BR: then block (else is block_target_else) */
    char *block_target_else;  /* for IR_COND_BR: else block name */
    char *result_ssa;  /* for IR_CALL when callee returns i32: SSA name for the result */
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

typedef enum { QOP_ALLOC_QREG, QOP_H, QOP_CX, QOP_MEASURE_ALL } QuantumOpKind;

typedef struct QuantumOp {
    QuantumOpKind kind;
    char *reg_name;   /* owned; for ALLOC, H, CX, MEASURE */
    size_t reg_size;  /* for ALLOC: qubit count */
    size_t index;     /* for H: qubit index */
    char *target_reg; /* owned; for CX: target register */
    size_t target_idx;/* for CX: target qubit index */
    size_t ctrl_idx;  /* for CX: control qubit index (control is reg_name) */
} QuantumOp;

typedef struct FunctionIr {
    char *name;       /* owned */
    ParamIr *params;  /* owned array; may be NULL if param_count == 0 */
    size_t param_count;
    char *return_type; /* owned */
    char **locals;    /* owned array of local var names; NULL if local_count == 0 */
    size_t local_count;
    BasicBlock *blocks;
    size_t block_count;
    QuantumOp *quantum_ops;  /* owned array; NULL if quantum_op_count == 0 */
    size_t quantum_op_count;
} FunctionIr;

typedef struct ModuleIr {
    FunctionIr *functions;
    size_t function_count;
} ModuleIr;

ModuleIr *lower_to_ir(const Program *program);
void ir_free(ModuleIr *module);

#endif /* QSCRIPT_IR_H */
