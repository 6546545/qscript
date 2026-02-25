#include "ir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static IrValue expr_to_ir_value(const Expr *e) {
    IrValue v = { IR_VAL_INT, NULL };
    if (!e) return v;
    if (e->kind == EXPR_INT && e->value) {
        v.kind = IR_VAL_INT;
        v.value = strdup(e->value);
    } else if (e->kind == EXPR_IDENT && e->value) {
        v.kind = IR_VAL_INT;
        v.value = strdup(e->value);
    } else if (e->kind == EXPR_STR && e->value) {
        v.kind = IR_VAL_STR;
        v.value = strdup(e->value);
    } else {
        v.value = strdup("0");
    }
    return v;
}

static void free_instruction(Instruction *inst) {
    if (!inst) return;
    free(inst->callee);
    free(inst->block_target);
    free(inst->block_target_else);
    if (inst->args) {
        for (size_t i = 0; i < inst->arg_count; i++) free(inst->args[i].value);
        free(inst->args);
    }
}

static void free_block(BasicBlock *b) {
    free(b->name);
    if (b->instructions) {
        for (size_t i = 0; i < b->instruction_count; i++) free_instruction(&b->instructions[i]);
        free(b->instructions);
    }
}

static void free_function_ir(FunctionIr *f) {
    free(f->name);
    free(f->return_type);
    if (f->params) {
        for (size_t i = 0; i < f->param_count; i++) {
            free(f->params[i].name);
            free(f->params[i].type);
        }
        free(f->params);
    }
    for (size_t i = 0; i < f->block_count; i++)
        free_block(&f->blocks[i]);
    free(f->blocks);
}

void ir_free(ModuleIr *module) {
    if (!module) return;
    for (size_t i = 0; i < module->function_count; i++)
        free_function_ir(&module->functions[i]);
    free(module->functions);
    module->functions = NULL;
    module->function_count = 0;
}

#define BLOCK_BUF_INIT 4
#define INST_BUF_INIT 16

typedef struct {
    BasicBlock *blocks;
    size_t block_count;
    size_t block_cap;
    Instruction *insts;
    size_t inst_count;
    size_t inst_cap;
    char *cur_block_name;
    size_t if_counter;
} BlockBuilder;

static void bb_init(BlockBuilder *bb) {
    memset(bb, 0, sizeof(BlockBuilder));
    bb->cur_block_name = strdup("entry");
    bb->block_cap = BLOCK_BUF_INIT;
    bb->blocks = (BasicBlock *)malloc(bb->block_cap * sizeof(BasicBlock));
    bb->inst_cap = INST_BUF_INIT;
    bb->insts = (Instruction *)malloc(bb->inst_cap * sizeof(Instruction));
}

static void bb_flush_block(BlockBuilder *bb) {
    if (!bb->cur_block_name || !bb->insts) return;
    if (bb->block_count >= bb->block_cap) {
        bb->block_cap *= 2;
        BasicBlock *n = (BasicBlock *)realloc(bb->blocks, bb->block_cap * sizeof(BasicBlock));
        if (!n) return;
        bb->blocks = n;
    }
    bb->blocks[bb->block_count].name = bb->cur_block_name;
    bb->blocks[bb->block_count].instructions = bb->insts;
    bb->blocks[bb->block_count].instruction_count = bb->inst_count;
    bb->block_count++;
    bb->cur_block_name = NULL;
    bb->insts = (Instruction *)malloc(bb->inst_cap * sizeof(Instruction));
    bb->inst_count = 0;
}

static int bb_add_inst(BlockBuilder *bb, Instruction *inst) {
    if (bb->inst_count >= bb->inst_cap) {
        bb->inst_cap *= 2;
        Instruction *n = (Instruction *)realloc(bb->insts, bb->inst_cap * sizeof(Instruction));
        if (!n) return -1;
        bb->insts = n;
    }
    bb->insts[bb->inst_count++] = *inst;
    memset(inst, 0, sizeof(Instruction));
    return 0;
}

static void bb_start_block(BlockBuilder *bb, const char *name) {
    bb_flush_block(bb);
    bb->cur_block_name = strdup(name);
}

static int lower_stmt_to_ir(const Statement *s, BlockBuilder *bb, const Program *program,
                            size_t func_idx, size_t stmt_start, size_t stmt_end);

static int lower_stmts_to_ir(const Statement *stmts, size_t count, BlockBuilder *bb,
                             const Program *program, size_t func_idx) {
    for (size_t j = 0; j < count; j++) {
        if (lower_stmt_to_ir(&stmts[j], bb, program, func_idx, j, count) != 0)
            return -1;
    }
    return 0;
}

static int lower_stmt_to_ir(const Statement *s, BlockBuilder *bb, const Program *program,
                            size_t func_idx, size_t stmt_start, size_t stmt_end) {
    (void)stmt_start;
    (void)stmt_end;
    if (s->kind == STMT_IF) {
        if (!s->init || s->init->kind != EXPR_BINARY) return -1;
        const Expr *cond = s->init;
        IrValue left = expr_to_ir_value(cond->base);
        IrValue right = expr_to_ir_value(cond->right);
        if (!left.value) left.value = strdup("0");
        if (!right.value) right.value = strdup("0");
        if (!left.value || !right.value) {
            free(left.value);
            free(right.value);
            return -1;
        }
        char then_name[32], else_name[32], merge_name[32];
        (void)snprintf(then_name, sizeof(then_name), "then.%lu", (unsigned long)bb->if_counter);
        (void)snprintf(else_name, sizeof(else_name), "else.%lu", (unsigned long)bb->if_counter);
        (void)snprintf(merge_name, sizeof(merge_name), "merge.%lu", (unsigned long)bb->if_counter);
        bb->if_counter++;

        Instruction cond_br = { 0 };
        cond_br.kind = IR_COND_BR;
        cond_br.callee = strdup(cond->value ? cond->value : "<");
        cond_br.arg_count = 2;
        cond_br.args = (IrValue *)malloc(2 * sizeof(IrValue));
        cond_br.args[0] = left;
        cond_br.args[1] = right;
        cond_br.block_target = strdup(then_name);
        cond_br.block_target_else = strdup(else_name);
        if (!cond_br.callee || !cond_br.args || !cond_br.block_target || !cond_br.block_target_else) {
            free_instruction(&cond_br);
            return -1;
        }
        if (bb_add_inst(bb, &cond_br) != 0) return -1;

        bb_start_block(bb, then_name);
        if (lower_stmts_to_ir(s->body, s->body_count, bb, program, func_idx) != 0) return -1;
        Instruction br_then = { 0 };
        br_then.kind = IR_BR;
        br_then.block_target = strdup(merge_name);
        if (!br_then.block_target) return -1;
        if (bb_add_inst(bb, &br_then) != 0) return -1;

        bb_start_block(bb, else_name);
        if (s->else_body && s->else_body_count > 0) {
            if (lower_stmts_to_ir(s->else_body, s->else_body_count, bb, program, func_idx) != 0) return -1;
        }
        Instruction br_else = { 0 };
        br_else.kind = IR_BR;
        br_else.block_target = strdup(merge_name);
        if (!br_else.block_target) return -1;
        if (bb_add_inst(bb, &br_else) != 0) return -1;

        bb_start_block(bb, merge_name);
        return 0;
    }
    if (s->kind == STMT_CALL) {
        Instruction inst = { 0 };
        inst.kind = IR_CALL;
        inst.callee = strdup(s->callee);
        inst.arg_count = s->arg_count;
        inst.args = (IrValue *)malloc(s->arg_count * sizeof(IrValue));
        if (!inst.callee || (s->arg_count > 0 && !inst.args)) {
            free_instruction(&inst);
            return -1;
        }
        for (size_t a = 0; a < s->arg_count; a++) {
            inst.args[a] = expr_to_ir_value(&s->args[a]);
            if (!inst.args[a].value) inst.args[a].value = strdup("0");
            if (!inst.args[a].value) {
                for (size_t b = 0; b < a; b++) free(inst.args[b].value);
                free_instruction(&inst);
                return -1;
            }
        }
        return bb_add_inst(bb, &inst);
    }
    if (s->kind == STMT_EXPR && s->init && s->init->kind == EXPR_CALL) {
        const Expr *call_expr = s->init;
        const char *callee = call_expr->base && call_expr->base->value ? call_expr->base->value : "?";
        size_t ac = call_expr->arg_count;
        Instruction inst = { 0 };
        inst.kind = IR_CALL;
        inst.callee = strdup(callee);
        inst.arg_count = ac;
        inst.args = (IrValue *)malloc(ac * sizeof(IrValue));
        if (!inst.callee || (ac > 0 && !inst.args)) {
            free_instruction(&inst);
            return -1;
        }
        for (size_t a = 0; a < ac; a++) {
            const Expr *arg = &call_expr->args[a];
            if (arg->kind == EXPR_STR && arg->value) {
                inst.args[a].kind = IR_VAL_STR;
                inst.args[a].value = strdup(arg->value);
            } else if (arg->kind == EXPR_IDENT && arg->value) {
                inst.args[a].kind = IR_VAL_INT;
                inst.args[a].value = strdup(arg->value);
            } else if (arg->kind == EXPR_INDEX && arg->base && arg->index) {
                inst.args[a].kind = IR_VAL_INT;
                inst.args[a].value = arg->index->value ? strdup(arg->index->value) : strdup("0");
            } else {
                inst.args[a] = expr_to_ir_value(arg);
                if (!inst.args[a].value) inst.args[a].value = strdup("0");
            }
            if (!inst.args[a].value) {
                for (size_t b = 0; b < a; b++) free(inst.args[b].value);
                free_instruction(&inst);
                return -1;
            }
        }
        return bb_add_inst(bb, &inst);
    }
    return 0;  /* skip let, quantum block for classical IR */
}

ModuleIr *lower_to_ir(const Program *program) {
    ModuleIr *m = (ModuleIr *)malloc(sizeof(ModuleIr));
    if (!m) return NULL;
    m->functions = (FunctionIr *)malloc(program->function_count * sizeof(FunctionIr));
    if (!m->functions) { free(m); return NULL; }
    m->function_count = program->function_count;

    for (size_t i = 0; i < program->function_count; i++) {
        const Function *f = &program->functions[i];
        FunctionIr *fir = &m->functions[i];
        fir->name = strdup(f->name);
        if (!fir->name) goto fail;
        fir->return_type = f->return_type ? strdup(f->return_type) : strdup("unit");
        if (!fir->return_type) goto fail;
        fir->param_count = f->param_count;
        fir->params = NULL;
        if (f->param_count > 0 && f->params) {
            fir->params = (ParamIr *)malloc(f->param_count * sizeof(ParamIr));
            if (!fir->params) goto fail;
            for (size_t p = 0; p < f->param_count; p++) {
                fir->params[p].name = strdup(f->params[p].name);
                fir->params[p].type = strdup(f->params[p].type);
                if (!fir->params[p].name || !fir->params[p].type) {
                    for (size_t q = 0; q <= p; q++) {
                        free(fir->params[q].name);
                        free(fir->params[q].type);
                    }
                    free(fir->params);
                    fir->params = NULL;
                    goto fail;
                }
            }
        }

        BlockBuilder bb;
        bb_init(&bb);
        if (!bb.blocks || !bb.insts) goto fail;

        if (lower_stmts_to_ir(f->body, f->body_count, &bb, program, i) != 0) {
            bb_flush_block(&bb);
            for (size_t k = 0; k < bb.block_count; k++) free_block(&bb.blocks[k]);
            free(bb.blocks);
            free(bb.insts);
            free(bb.cur_block_name);
            goto fail;
        }

        Instruction ret_inst = { 0 };
        ret_inst.kind = IR_RET;
        if (bb_add_inst(&bb, &ret_inst) != 0) {
            bb_flush_block(&bb);
            for (size_t k = 0; k < bb.block_count; k++) free_block(&bb.blocks[k]);
            free(bb.blocks);
            free(bb.insts);
            free(bb.cur_block_name);
            goto fail;
        }

        bb_flush_block(&bb);
        fir->block_count = bb.block_count;
        fir->blocks = bb.blocks;
        free(bb.insts);
        free(bb.cur_block_name);
    }
    return m;

fail:
    ir_free(m);
    return NULL;
}
