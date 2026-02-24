#include "ir.h"
#include <stdlib.h>
#include <string.h>

static void free_instruction(Instruction *inst) {
    if (!inst) return;
    free(inst->callee);
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

ModuleIr *lower_to_ir(const Program *program) {
    ModuleIr *m = (ModuleIr *)malloc(sizeof(ModuleIr));
    if (!m) return NULL;
    m->functions = (FunctionIr *)malloc(program->function_count * sizeof(FunctionIr));
    if (!m->functions) { free(m); return NULL; }
    m->function_count = program->function_count;

    Instruction *insts = NULL;
    size_t inst_count = 0;

    for (size_t i = 0; i < program->function_count; i++) {
        const Function *f = &program->functions[i];
        FunctionIr *fir = &m->functions[i];
        fir->name = strdup(f->name);
        if (!fir->name) goto fail;

        size_t inst_cap = 16;
        inst_count = 0;
        insts = (Instruction *)malloc(inst_cap * sizeof(Instruction));
        if (!insts) goto fail;

        for (size_t j = 0; j < f->body_count; j++) {
            const Statement *s = &f->body[j];
            if (s->kind != STMT_CALL) continue;
            if (inst_count >= inst_cap) {
                size_t new_cap = inst_cap * 2;
                Instruction *n = (Instruction *)realloc(insts, new_cap * sizeof(Instruction));
                if (!n) goto fail_insts;
                insts = n;
                inst_cap = new_cap;
            }
            insts[inst_count].kind = IR_CALL;
            insts[inst_count].callee = strdup(s->callee);
            if (!insts[inst_count].callee) goto fail_insts;
            insts[inst_count].arg_count = s->arg_count;
            insts[inst_count].args = (IrValue *)malloc(s->arg_count * sizeof(IrValue));
            if (!insts[inst_count].args && s->arg_count > 0) {
                free(insts[inst_count].callee);
                goto fail_insts;
            }
            for (size_t a = 0; a < s->arg_count; a++) {
                if (s->args[a].kind == EXPR_STR) {
                    insts[inst_count].args[a].kind = IR_VAL_STR;
                    insts[inst_count].args[a].value = strdup(s->args[a].value);
                } else {
                    insts[inst_count].args[a].kind = IR_VAL_INT;
                    insts[inst_count].args[a].value = strdup(s->args[a].value);
                }
                if (!insts[inst_count].args[a].value) {
                    for (size_t b = 0; b < a; b++) free(insts[inst_count].args[b].value);
                    free(insts[inst_count].args);
                    free(insts[inst_count].callee);
                    goto fail_insts;
                }
            }
            inst_count++;
        }

        if (inst_count >= inst_cap) {
            Instruction *n = (Instruction *)realloc(insts, (inst_cap + 1) * sizeof(Instruction));
            if (!n) goto fail_insts;
            insts = n;
        }
        insts[inst_count].kind = IR_RET;
        insts[inst_count].callee = NULL;
        insts[inst_count].args = NULL;
        insts[inst_count].arg_count = 0;
        inst_count++;

        fir->block_count = 1;
        fir->blocks = (BasicBlock *)malloc(sizeof(BasicBlock));
        if (!fir->blocks) goto fail_insts;
        fir->blocks[0].name = strdup("entry");
        fir->blocks[0].instructions = insts;
        fir->blocks[0].instruction_count = inst_count;
        insts = NULL; /* ownership transferred to block */
    }
    return m;

fail_insts:
    if (insts) {
        for (size_t k = 0; k < inst_count; k++) free_instruction(&insts[k]);
        free(insts);
    }
fail:
    ir_free(m);
    return NULL;
}
