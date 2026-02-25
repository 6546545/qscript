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
    } else if (e->kind == EXPR_BOOL && e->value) {
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
    free(inst->result_ssa);
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
    if (f->locals) {
        for (size_t i = 0; i < f->local_count; i++) free(f->locals[i]);
        free(f->locals);
    }
    for (size_t i = 0; i < f->block_count; i++)
        free_block(&f->blocks[i]);
    free(f->blocks);
    if (f->quantum_ops) {
        for (size_t q = 0; q < f->quantum_op_count; q++) {
            free(f->quantum_ops[q].reg_name);
            free(f->quantum_ops[q].target_reg);
        }
        free(f->quantum_ops);
    }
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
    size_t loop_counter;
    size_t add_counter;
    char **locals;
    size_t local_count;
    size_t local_cap;
    char *break_target;    /* when inside loop: block to branch to on break */
    char *continue_target; /* when inside loop: block to branch to on continue (loop head) */
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
static int is_comparison_op(const char *op);

/* Returns 0=ok, 1=saw return (stop), -1=error */
static int lower_stmts_to_ir(const Statement *stmts, size_t count, BlockBuilder *bb,
                             const Program *program, size_t func_idx) {
    for (size_t j = 0; j < count; j++) {
        int r = lower_stmt_to_ir(&stmts[j], bb, program, func_idx, j, count);
        if (r == -1) return -1;
        if (r == 1) return 1;  /* saw return, stop */
        if (r == 2) return 2;  /* saw break, stop loop body */
        if (r == 3) return 3;  /* saw continue, stop loop body */
    }
    return 0;
}

/* Evaluate expression to IrValue; for EXPR_BINARY with +,-,*,/ emits IR_ADD and returns IR_VAL_SSA. */
static int eval_expr_to_value(const Expr *e, BlockBuilder *bb, const Program *program,
                              size_t func_idx, IrValue *out) {
    if (!e || !out) return -1;
    memset(out, 0, sizeof(IrValue));
    if (e->kind == EXPR_INT || e->kind == EXPR_BOOL || e->kind == EXPR_IDENT || e->kind == EXPR_STR) {
        *out = expr_to_ir_value(e);
        if (!out->value) out->value = strdup("0");
        return out->value ? 0 : -1;
    }
    if (e->kind == EXPR_BINARY && e->value && e->base) {
        if (strcmp(e->value, "!") == 0 && !e->right) {
            /* Unary !: 0 if operand non-zero, 1 if operand zero */
            IrValue arg;
            if (eval_expr_to_value(e->base, bb, program, func_idx, &arg) != 0) return -1;
            if (!arg.value) arg.value = strdup("0");
            if (!arg.value) return -1;
            char ssa_name[32];
            (void)snprintf(ssa_name, sizeof(ssa_name), "not.%lu", (unsigned long)bb->add_counter);
            bb->add_counter++;
            Instruction not_inst = { 0 };
            not_inst.kind = IR_NOT;
            not_inst.arg_count = 1;
            not_inst.args = (IrValue *)malloc(sizeof(IrValue));
            not_inst.result_ssa = strdup(ssa_name);
            if (!not_inst.args || !not_inst.result_ssa) {
                free(arg.value);
                free(not_inst.args);
                free(not_inst.result_ssa);
                return -1;
            }
            not_inst.args[0] = arg;
            if (bb_add_inst(bb, &not_inst) != 0) return -1;
            out->kind = IR_VAL_SSA;
            out->value = strdup(ssa_name);
            return out->value ? 0 : -1;
        }
        if (!e->right) return -1;
        IrValue left_val, right_val;
        if (eval_expr_to_value(e->base, bb, program, func_idx, &left_val) != 0) return -1;
        if (eval_expr_to_value(e->right, bb, program, func_idx, &right_val) != 0) {
            free(left_val.value);
            return -1;
        }
        if (!left_val.value) left_val.value = strdup("0");
        if (!right_val.value) right_val.value = strdup("0");
        if (!left_val.value || !right_val.value) {
            free(left_val.value);
            free(right_val.value);
            return -1;
        }
        const char *op = e->value;
        if (is_comparison_op(op)) {
            /* Comparison as value: produce i32 0 or 1 */
            char ssa_name[32];
            (void)snprintf(ssa_name, sizeof(ssa_name), "icmp.%lu", (unsigned long)bb->add_counter);
            bb->add_counter++;
            Instruction icmp_inst = { 0 };
            icmp_inst.kind = IR_ICMP;
            icmp_inst.callee = strdup(op);
            icmp_inst.arg_count = 2;
            icmp_inst.args = (IrValue *)malloc(2 * sizeof(IrValue));
            icmp_inst.result_ssa = strdup(ssa_name);
            if (!icmp_inst.callee || !icmp_inst.args || !icmp_inst.result_ssa) {
                free(left_val.value);
                free(right_val.value);
                free_instruction(&icmp_inst);
                return -1;
            }
            icmp_inst.args[0] = left_val;
            icmp_inst.args[1] = right_val;
            if (bb_add_inst(bb, &icmp_inst) != 0) return -1;
            out->kind = IR_VAL_SSA;
            out->value = strdup(ssa_name);
            return out->value ? 0 : -1;
        }
        if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
            strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
            char ssa_name[32];
            (void)snprintf(ssa_name, sizeof(ssa_name), "binop.%lu", (unsigned long)bb->add_counter);
            bb->add_counter++;
            Instruction add_inst = { 0 };
            add_inst.kind = IR_ADD;
            add_inst.callee = strdup(op);
            add_inst.arg_count = 2;
            add_inst.args = (IrValue *)malloc(2 * sizeof(IrValue));
            if (!add_inst.callee || !add_inst.args) {
                free(left_val.value);
                free(right_val.value);
                return -1;
            }
            add_inst.args[0] = left_val;
            add_inst.args[1] = right_val;
            if (bb_add_inst(bb, &add_inst) != 0) return -1;
            out->kind = IR_VAL_SSA;
            out->value = strdup(ssa_name);
            return out->value ? 0 : -1;
        }
        free(left_val.value);
        free(right_val.value);
    }
    if (e->kind == EXPR_CALL && e->base && e->base->value) {
        const char *callee = e->base->value;
        for (size_t fi = 0; fi < program->function_count; fi++) {
            if (strcmp(program->functions[fi].name, callee) == 0 &&
                program->functions[fi].return_type &&
                (strcmp(program->functions[fi].return_type, "i32") == 0 || strcmp(program->functions[fi].return_type, "bool") == 0)) {
                /* Emit call that produces i32 result */
                size_t ac = e->arg_count;
                Instruction call_inst = { 0 };
                call_inst.kind = IR_CALL;
                call_inst.callee = strdup(callee);
                call_inst.arg_count = ac;
                call_inst.args = (IrValue *)malloc(ac * sizeof(IrValue));
                char ssa_name[32];
                (void)snprintf(ssa_name, sizeof(ssa_name), "call.%lu", (unsigned long)bb->add_counter);
                bb->add_counter++;
                call_inst.result_ssa = strdup(ssa_name);
                if (!call_inst.callee || (ac > 0 && !call_inst.args) || !call_inst.result_ssa) {
                    free_instruction(&call_inst);
                    return -1;
                }
                for (size_t a = 0; a < ac; a++) {
                    if (eval_expr_to_value(&e->args[a], bb, program, func_idx, &call_inst.args[a]) != 0) {
                        free_instruction(&call_inst);
                        return -1;
                    }
                    if (!call_inst.args[a].value) call_inst.args[a].value = strdup("0");
                    if (!call_inst.args[a].value) {
                        free_instruction(&call_inst);
                        return -1;
                    }
                }
                if (bb_add_inst(bb, &call_inst) != 0) return -1;
                out->kind = IR_VAL_SSA;
                out->value = strdup(ssa_name);
                return out->value ? 0 : -1;
            }
        }
        /* Built-in or non-i32: fall through to default */
    }
    out->kind = IR_VAL_INT;
    out->value = strdup("0");
    return out->value ? 0 : -1;
}

static int add_local(BlockBuilder *bb, const char *name) {
    if (bb->local_count >= bb->local_cap) {
        bb->local_cap = bb->local_cap ? bb->local_cap * 2 : 4;
        char **n = (char **)realloc(bb->locals, bb->local_cap * sizeof(char *));
        if (!n) return -1;
        bb->locals = n;
    }
    bb->locals[bb->local_count++] = strdup(name);
    return bb->locals[bb->local_count - 1] ? 0 : -1;
}

static int is_comparison_op(const char *op) {
    return op && (strcmp(op, "<") == 0 || strcmp(op, ">") == 0 || strcmp(op, "<=") == 0 ||
                  strcmp(op, ">=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0);
}

/* Emit condition to IR with short-circuit for && and ||. Branches: true->then_name, false->else_name. */
static int emit_cond_to_ir(const Expr *cond, const char *then_name, const char *else_name,
                           BlockBuilder *bb, const Program *program, size_t func_idx) {
    if (!cond) return -1;
    if (cond->kind == EXPR_BINARY && cond->value) {
        if (strcmp(cond->value, "!") == 0 && cond->base) {
            /* !expr: swap then and else */
            return emit_cond_to_ir(cond->base, else_name, then_name, bb, program, func_idx);
        }
        if (strcmp(cond->value, "&&") == 0) {
            /* left && right: if !left goto else; if !right goto else; goto then */
            char and_right[32];
            (void)snprintf(and_right, sizeof(and_right), "and.%lu", (unsigned long)bb->if_counter);
            if (emit_cond_to_ir(cond->base, and_right, else_name, bb, program, func_idx) != 0) return -1;
            bb_start_block(bb, and_right);
            return emit_cond_to_ir(cond->right, then_name, else_name, bb, program, func_idx);
        }
        if (strcmp(cond->value, "||") == 0) {
            /* left || right: if left goto then; if right goto then; goto else */
            char or_right[32];
            (void)snprintf(or_right, sizeof(or_right), "or.%lu", (unsigned long)bb->if_counter);
            if (emit_cond_to_ir(cond->base, then_name, or_right, bb, program, func_idx) != 0) return -1;
            bb_start_block(bb, or_right);
            return emit_cond_to_ir(cond->right, then_name, else_name, bb, program, func_idx);
        }
        if (is_comparison_op(cond->value) && cond->base && cond->right) {
            IrValue left = expr_to_ir_value(cond->base);
            IrValue right = expr_to_ir_value(cond->right);
            if (!left.value) left.value = strdup("0");
            if (!right.value) right.value = strdup("0");
            if (!left.value || !right.value) {
                free(left.value);
                free(right.value);
                return -1;
            }
            Instruction cond_br = { 0 };
            cond_br.kind = IR_COND_BR;
            cond_br.callee = strdup(cond->value);
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
            return bb_add_inst(bb, &cond_br);
        }
    }
    /* Bare value or arithmetic: evaluate and treat as value != 0 */
    IrValue left, right = { IR_VAL_INT, strdup("0") };
    if (!right.value) return -1;
    if (eval_expr_to_value(cond, bb, program, func_idx, &left) != 0) {
        free(right.value);
        return -1;
    }
    if (!left.value) left.value = strdup("0");
    Instruction cond_br = { 0 };
    cond_br.kind = IR_COND_BR;
    cond_br.callee = strdup("!=");
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
    return bb_add_inst(bb, &cond_br);
}

/* Returns 0=ok, 1=saw return (caller should stop), -1=error */
static int lower_stmt_to_ir(const Statement *s, BlockBuilder *bb, const Program *program,
                            size_t func_idx, size_t stmt_start, size_t stmt_end) {
    (void)stmt_start;
    (void)stmt_end;
    if (s->kind == STMT_BREAK) {
        if (!bb->break_target) return -1;  /* break outside loop */
        Instruction br_inst = { 0 };
        br_inst.kind = IR_BR;
        br_inst.block_target = strdup(bb->break_target);
        if (!br_inst.block_target) return -1;
        if (bb_add_inst(bb, &br_inst) != 0) return -1;
        return 2;  /* saw break - stop loop body, don't add back-edge */
    }
    if (s->kind == STMT_CONTINUE) {
        if (!bb->continue_target) return -1;  /* continue outside loop */
        Instruction br_inst = { 0 };
        br_inst.kind = IR_BR;
        br_inst.block_target = strdup(bb->continue_target);
        if (!br_inst.block_target) return -1;
        if (bb_add_inst(bb, &br_inst) != 0) return -1;
        return 3;  /* saw continue - stop loop body, branch to head */
    }
    if (s->kind == STMT_RETURN) {
        Instruction ret_inst = { 0 };
        ret_inst.kind = IR_RET;
        if (s->init) {
            IrValue val;
            if (eval_expr_to_value(s->init, bb, program, func_idx, &val) != 0) return -1;
            if (!val.value) val.value = strdup("0");
            if (!val.value) return -1;
            ret_inst.arg_count = 1;
            ret_inst.args = (IrValue *)malloc(sizeof(IrValue));
            if (!ret_inst.args) { free(val.value); return -1; }
            ret_inst.args[0] = val;
        }
        if (bb_add_inst(bb, &ret_inst) != 0) return -1;
        return 1;  /* stop processing */
    }
    if (s->kind == STMT_LOOP) {
        char loop_head[32], loop_exit[32];
        (void)snprintf(loop_head, sizeof(loop_head), "loop.%lu", (unsigned long)bb->loop_counter);
        (void)snprintf(loop_exit, sizeof(loop_exit), "loop_exit.%lu", (unsigned long)bb->loop_counter);
        bb->loop_counter++;
        Instruction br_to_loop = { 0 };
        br_to_loop.kind = IR_BR;
        br_to_loop.block_target = strdup(loop_head);
        if (!br_to_loop.block_target) return -1;
        if (bb_add_inst(bb, &br_to_loop) != 0) return -1;
        bb_start_block(bb, loop_head);
        char *saved_break = bb->break_target;
        char *saved_continue = bb->continue_target;
        bb->break_target = strdup(loop_exit);
        bb->continue_target = strdup(loop_head);
        if (!bb->break_target || !bb->continue_target) return -1;
        int r = lower_stmts_to_ir(s->body, s->body_count, bb, program, func_idx);
        free(bb->break_target);
        free(bb->continue_target);
        bb->break_target = saved_break;
        bb->continue_target = saved_continue;
        if (r == 1) return 1;   /* return inside loop - propagate */
        if (r == -1) return -1;
        if (r == 0 || r == 3) {
            /* normal loop end - add back-edge */
            Instruction br_back = { 0 };
            br_back.kind = IR_BR;
            br_back.block_target = strdup(loop_head);
            if (!br_back.block_target) return -1;
            if (bb_add_inst(bb, &br_back) != 0) return -1;
        }
        /* r==2: break - already branched to loop_exit, no back-edge */
        /* r==3: continue - already branched to loop_head, no back-edge */
        bb_start_block(bb, loop_exit);
        return 0;
    }
    if (s->kind == STMT_WHILE && s->init) {
        /* while (cond) { body } => loop { if !cond break; body } */
        char loop_head[32], loop_exit[32], cond_then[32];
        (void)snprintf(loop_head, sizeof(loop_head), "while.%lu", (unsigned long)bb->loop_counter);
        (void)snprintf(loop_exit, sizeof(loop_exit), "while_exit.%lu", (unsigned long)bb->loop_counter);
        (void)snprintf(cond_then, sizeof(cond_then), "while_body.%lu", (unsigned long)bb->if_counter);
        bb->loop_counter++;
        Instruction br_to_loop = { 0 };
        br_to_loop.kind = IR_BR;
        br_to_loop.block_target = strdup(loop_head);
        if (!br_to_loop.block_target) return -1;
        if (bb_add_inst(bb, &br_to_loop) != 0) return -1;
        bb_start_block(bb, loop_head);
        if (emit_cond_to_ir(s->init, cond_then, loop_exit, bb, program, func_idx) != 0) return -1;
        bb_start_block(bb, cond_then);
        char *saved_break = bb->break_target;
        char *saved_continue = bb->continue_target;
        bb->break_target = strdup(loop_exit);
        bb->continue_target = strdup(loop_head);
        if (!bb->break_target || !bb->continue_target) return -1;
        int r = lower_stmts_to_ir(s->body, s->body_count, bb, program, func_idx);
        free(bb->break_target);
        free(bb->continue_target);
        bb->break_target = saved_break;
        bb->continue_target = saved_continue;
        if (r == 1) return 1;
        if (r == -1) return -1;
        if (r == 0 || r == 3) {
            Instruction br_back = { 0 };
            br_back.kind = IR_BR;
            br_back.block_target = strdup(loop_head);
            if (!br_back.block_target) return -1;
            if (bb_add_inst(bb, &br_back) != 0) return -1;
        }
        bb_start_block(bb, loop_exit);
        return 0;
    }
    if (s->kind == STMT_FOR) {
        /* for (init; cond; step) { body } => init; loop { if !cond break; body; step } */
        if (s->for_init) {
            int ri = lower_stmt_to_ir(s->for_init, bb, program, func_idx, 0, 1);
            if (ri == -1) return -1;
        }
        char loop_head[32], loop_exit[32];
        (void)snprintf(loop_head, sizeof(loop_head), "for.%lu", (unsigned long)bb->loop_counter);
        (void)snprintf(loop_exit, sizeof(loop_exit), "for_exit.%lu", (unsigned long)bb->loop_counter);
        bb->loop_counter++;
        Instruction br_to_loop = { 0 };
        br_to_loop.kind = IR_BR;
        br_to_loop.block_target = strdup(loop_head);
        if (!br_to_loop.block_target) return -1;
        if (bb_add_inst(bb, &br_to_loop) != 0) return -1;
        bb_start_block(bb, loop_head);
        if (s->init) {
            char cond_then[32];
            (void)snprintf(cond_then, sizeof(cond_then), "for_body.%lu", (unsigned long)bb->if_counter);
            if (emit_cond_to_ir(s->init, cond_then, loop_exit, bb, program, func_idx) != 0) return -1;
            bb_start_block(bb, cond_then);
        }
        char *saved_break = bb->break_target;
        char *saved_continue = bb->continue_target;
        bb->break_target = strdup(loop_exit);
        bb->continue_target = strdup(loop_head);
        if (!bb->break_target || !bb->continue_target) return -1;
        int r = lower_stmts_to_ir(s->body, s->body_count, bb, program, func_idx);
        free(bb->break_target);
        free(bb->continue_target);
        bb->break_target = saved_break;
        bb->continue_target = saved_continue;
        if (r == 1) return 1;
        if (r == -1) return -1;
        if (r == 0 || r == 3) {
            if (s->for_step) {
                int rs = lower_stmt_to_ir(s->for_step, bb, program, func_idx, 0, 1);
                if (rs == -1) return -1;
            }
            Instruction br_back = { 0 };
            br_back.kind = IR_BR;
            br_back.block_target = strdup(loop_head);
            if (!br_back.block_target) return -1;
            if (bb_add_inst(bb, &br_back) != 0) return -1;
        }
        bb_start_block(bb, loop_exit);
        return 0;
    }
    if (s->kind == STMT_ASSIGN && s->let_name && s->init) {
        IrValue val;
        if (eval_expr_to_value(s->init, bb, program, func_idx, &val) != 0) return -1;
        if (!val.value) val.value = strdup("0");
        if (!val.value) return -1;
        /* Variable must already exist (from let or param) - just store */
        Instruction store = { 0 };
        store.kind = IR_STORE;
        store.callee = strdup(s->let_name);
        store.arg_count = 1;
        store.args = (IrValue *)malloc(sizeof(IrValue));
        if (!store.callee || !store.args) {
            free(val.value);
            return -1;
        }
        store.args[0] = val;
        if (bb_add_inst(bb, &store) != 0) return -1;
        return 0;
    }
    if (s->kind == STMT_LET && s->let_name && s->init) {
        const Expr *init = s->init;
        IrValue val;
        if (eval_expr_to_value(init, bb, program, func_idx, &val) != 0) return -1;
        if (!val.value) val.value = strdup("0");
        if (!val.value) return -1;
        if (add_local(bb, s->let_name) != 0) { free(val.value); return -1; }
        Instruction alloc = { 0 };
        alloc.kind = IR_ALLOCA;
        alloc.callee = strdup(s->let_name);
        if (!alloc.callee) { free(val.value); return -1; }
        if (bb_add_inst(bb, &alloc) != 0) return -1;
        Instruction store = { 0 };
        store.kind = IR_STORE;
        store.callee = strdup(s->let_name);
        store.arg_count = 1;
        store.args = (IrValue *)malloc(sizeof(IrValue));
        if (!store.callee || !store.args) {
            free(val.value);
            return -1;
        }
        store.args[0] = val;
        if (bb_add_inst(bb, &store) != 0) return -1;
        return 0;
    }
    if (s->kind == STMT_IF) {
        if (!s->init) return -1;
        const Expr *cond = s->init;
        char then_name[32], else_name[32], merge_name[32];
        (void)snprintf(then_name, sizeof(then_name), "then.%lu", (unsigned long)bb->if_counter);
        (void)snprintf(else_name, sizeof(else_name), "else.%lu", (unsigned long)bb->if_counter);
        (void)snprintf(merge_name, sizeof(merge_name), "merge.%lu", (unsigned long)bb->if_counter);
        bb->if_counter++;

        /* Emit condition: supports comparison, &&, || */
        if (emit_cond_to_ir(cond, then_name, else_name, bb, program, func_idx) != 0) return -1;

        bb_start_block(bb, then_name);
        int then_ret = lower_stmts_to_ir(s->body, s->body_count, bb, program, func_idx);
        if (then_ret == -1) return -1;
        if (then_ret == 0) {
            Instruction br_then = { 0 };
            br_then.kind = IR_BR;
            br_then.block_target = strdup(merge_name);
            if (!br_then.block_target) return -1;
            if (bb_add_inst(bb, &br_then) != 0) return -1;
        }

        bb_start_block(bb, else_name);
        int else_ret = 0;
        if (s->else_body && s->else_body_count > 0) {
            else_ret = lower_stmts_to_ir(s->else_body, s->else_body_count, bb, program, func_idx);
            if (else_ret == -1) return -1;
        }
        if (else_ret == 0) {
            Instruction br_else = { 0 };
            br_else.kind = IR_BR;
            br_else.block_target = strdup(merge_name);
            if (!br_else.block_target) return -1;
            if (bb_add_inst(bb, &br_else) != 0) return -1;
        }

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

/* Extract size from "qreg<2>" or "alloc_qreg<2>" - returns 0 on parse failure */
static size_t parse_qreg_size(const char *s) {
    if (!s) return 0;
    const char *p = strchr(s, '<');
    if (!p || !p[1]) return 0;
    size_t n = 0;
    for (p++; *p >= '0' && *p <= '9'; p++) n = n * 10 + (size_t)(*p - '0');
    return (*p == '>') ? n : 0;
}

/* Get qubit ref from expr: q[0] -> reg_name, index. Returns 0 on success. */
static int get_qubit_ref(const Expr *e, char **reg_name, size_t *index) {
    if (!e || !reg_name || !index) return -1;
    if (e->kind != EXPR_INDEX || !e->base || !e->index) return -1;
    if (e->base->kind != EXPR_IDENT || !e->base->value) return -1;
    *reg_name = strdup(e->base->value);
    if (!*reg_name) return -1;
    *index = e->index->kind == EXPR_INT && e->index->value ? (size_t)atoi(e->index->value) : 0;
    return 0;
}

/* Lower quantum block body to quantum_ops. Returns 0 on success. */
static int lower_quantum_block(const Statement *stmts, size_t count, QuantumOp **out_ops, size_t *out_count) {
    QuantumOp *ops = NULL;
    size_t cap = 0, n = 0;
    for (size_t j = 0; j < count; j++) {
        const Statement *s = &stmts[j];
        if (s->kind == STMT_QUANTUM_BLOCK) {
            if (lower_quantum_block(s->body, s->body_count, &ops, &n) != 0) goto fail;
            continue;
        }
        if (s->kind == STMT_LET && s->let_name && s->init && s->init->kind == EXPR_CALL &&
            s->init->base && s->init->base->value &&
            strstr(s->init->base->value, "alloc_qreg") != NULL) {
            size_t sz = parse_qreg_size(s->let_type ? s->let_type : s->init->base->value);
            if (sz == 0) sz = 2;
            if (n >= cap) {
                cap = cap ? cap * 2 : 4;
                QuantumOp *nn = (QuantumOp *)realloc(ops, cap * sizeof(QuantumOp));
                if (!nn) goto fail;
                ops = nn;
            }
            memset(&ops[n], 0, sizeof(QuantumOp));
            ops[n].kind = QOP_ALLOC_QREG;
            ops[n].reg_name = strdup(s->let_name);
            ops[n].reg_size = sz;
            if (!ops[n].reg_name) goto fail;
            n++;
            continue;
        }
        if (s->kind == STMT_LET && s->init && s->init->kind == EXPR_CALL &&
            s->init->base && s->init->base->value &&
            strcmp(s->init->base->value, "measure_all") == 0 &&
            s->init->arg_count >= 1) {
            const Expr *arg = &s->init->args[0];
            char *r = NULL;
            size_t idx = 0;
            if (arg->kind == EXPR_IDENT && arg->value) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 4;
                    QuantumOp *nn = (QuantumOp *)realloc(ops, cap * sizeof(QuantumOp));
                    if (!nn) goto fail;
                    ops = nn;
                }
                memset(&ops[n], 0, sizeof(QuantumOp));
                ops[n].kind = QOP_MEASURE_ALL;
                ops[n].reg_name = strdup(arg->value);
                if (!ops[n].reg_name) goto fail;
                n++;
            } else if (get_qubit_ref(arg, &r, &idx) == 0) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 4;
                    QuantumOp *nn = (QuantumOp *)realloc(ops, cap * sizeof(QuantumOp));
                    if (!nn) { free(r); goto fail; }
                    ops = nn;
                }
                memset(&ops[n], 0, sizeof(QuantumOp));
                ops[n].kind = QOP_MEASURE_ALL;
                ops[n].reg_name = r;
                n++;
            }
            continue;
        }
        if ((s->kind == STMT_CALL || (s->kind == STMT_EXPR && s->init && s->init->kind == EXPR_CALL))) {
            const char *callee = s->kind == STMT_CALL ? s->callee : (s->init->base && s->init->base->value ? s->init->base->value : NULL);
            const Expr *a0 = s->kind == STMT_CALL ? (s->arg_count > 0 ? &s->args[0] : NULL) : (s->init->arg_count > 0 ? &s->init->args[0] : NULL);
            const Expr *a1 = s->kind == STMT_CALL ? (s->arg_count > 1 ? &s->args[1] : NULL) : (s->init->arg_count > 1 ? &s->init->args[1] : NULL);
            if (callee && strcmp(callee, "h") == 0 && a0) {
                char *r = NULL;
                size_t idx = 0;
                if (get_qubit_ref(a0, &r, &idx) == 0) {
                    if (n >= cap) {
                        cap = cap ? cap * 2 : 4;
                        QuantumOp *nn = (QuantumOp *)realloc(ops, cap * sizeof(QuantumOp));
                        if (!nn) { free(r); goto fail; }
                        ops = nn;
                    }
                    memset(&ops[n], 0, sizeof(QuantumOp));
                    ops[n].kind = QOP_H;
                    ops[n].reg_name = r;
                    ops[n].index = idx;
                    n++;
                }
            } else if (callee && strcmp(callee, "cx") == 0 && a0 && a1) {
                char *cr = NULL, *tr = NULL;
                size_t ci = 0, ti = 0;
                if (get_qubit_ref(a0, &cr, &ci) == 0 && get_qubit_ref(a1, &tr, &ti) == 0) {
                    if (n >= cap) {
                        cap = cap ? cap * 2 : 4;
                        QuantumOp *nn = (QuantumOp *)realloc(ops, cap * sizeof(QuantumOp));
                        if (!nn) { free(cr); free(tr); goto fail; }
                        ops = nn;
                    }
                    memset(&ops[n], 0, sizeof(QuantumOp));
                    ops[n].kind = QOP_CX;
                    ops[n].reg_name = cr;
                    ops[n].ctrl_idx = ci;
                    ops[n].target_reg = tr;
                    ops[n].target_idx = ti;
                    n++;
                }
            }
        }
    }
    *out_ops = ops;
    *out_count = n;
    return 0;
fail:
    if (ops) {
        for (size_t k = 0; k < n; k++) {
            free(ops[k].reg_name);
            free(ops[k].target_reg);
        }
        free(ops);
    }
    return -1;
}

/* Resolve type alias; returns newly allocated string or NULL. Depth limits recursion. */
static char *resolve_type(const Program *program, const char *name, int depth) {
    if (!name) return NULL;
    if (depth > 8) return strdup(name);
    if (!program->type_aliases) return strdup(name);
    for (size_t i = 0; i < program->type_alias_count; i++) {
        if (program->type_aliases[i].name && strcmp(program->type_aliases[i].name, name) == 0) {
            char *resolved = resolve_type(program, program->type_aliases[i].value, depth + 1);
            return resolved ? resolved : strdup(name);
        }
    }
    return strdup(name);
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
        fir->return_type = resolve_type(program, f->return_type, 0);
        if (!fir->return_type) fir->return_type = strdup("unit");
        if (!fir->return_type) goto fail;
        fir->param_count = f->param_count;
        fir->params = NULL;
        if (f->param_count > 0 && f->params) {
            fir->params = (ParamIr *)malloc(f->param_count * sizeof(ParamIr));
            if (!fir->params) goto fail;
            for (size_t p = 0; p < f->param_count; p++) {
                fir->params[p].name = strdup(f->params[p].name);
                fir->params[p].type = resolve_type(program, f->params[p].type, 0);
                if (!fir->params[p].type) fir->params[p].type = strdup(f->params[p].type);
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

        int body_ret = lower_stmts_to_ir(f->body, f->body_count, &bb, program, i);
        if (body_ret == -1) {
            bb_flush_block(&bb);
            for (size_t k = 0; k < bb.block_count; k++) free_block(&bb.blocks[k]);
            free(bb.blocks);
            free(bb.insts);
            free(bb.cur_block_name);
            goto fail;
        }

        /* Add implicit ret only if no explicit return was seen */
        if (body_ret == 0) {
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
        }

        bb_flush_block(&bb);
        fir->block_count = bb.block_count;
        fir->blocks = bb.blocks;
        fir->local_count = bb.local_count;
        fir->locals = bb.locals;
        free(bb.insts);
        free(bb.cur_block_name);

        /* Extract quantum ops from function body */
        fir->quantum_ops = NULL;
        fir->quantum_op_count = 0;
        if (lower_quantum_block(f->body, f->body_count, &fir->quantum_ops, &fir->quantum_op_count) != 0) {
            /* Non-fatal: quantum block may be empty or unsupported */
            fir->quantum_ops = NULL;
            fir->quantum_op_count = 0;
        }
    }
    return m;

fail:
    ir_free(m);
    return NULL;
}
