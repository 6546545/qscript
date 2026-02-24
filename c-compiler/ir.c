#include "ir.h"
#include <stdlib.h>
#include <string.h>

static void free_block(BasicBlock *b) {
    free(b->name);
    free(b->instructions);
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
    m->functions = NULL;
    m->function_count = 0;

    m->functions = (FunctionIr *)malloc(program->function_count * sizeof(FunctionIr));
    if (!m->functions) { free(m); return NULL; }
    m->function_count = program->function_count;

    for (size_t i = 0; i < program->function_count; i++) {
        FunctionIr *f = &m->functions[i];
        f->name = strdup(program->functions[i].name);
        if (!f->name) goto fail;
        f->block_count = 1;
        f->blocks = (BasicBlock *)malloc(sizeof(BasicBlock));
        if (!f->blocks) goto fail;
        f->blocks[0].name = strdup("entry");
        if (!f->blocks[0].name) { free(f->blocks); goto fail; }
        f->blocks[0].instruction_count = 1;
        f->blocks[0].instructions = (Instruction *)malloc(sizeof(Instruction));
        if (!f->blocks[0].instructions) {
            free(f->blocks[0].name);
            free(f->blocks);
            goto fail;
        }
        f->blocks[0].instructions[0].kind = IR_NOP;
    }
    return m;

fail:
    ir_free(m);
    return NULL;
}
