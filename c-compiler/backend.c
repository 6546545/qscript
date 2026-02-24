#include "backend.h"
#include <stdio.h>

void backend_classical_emit(const ModuleIr *module) {
    printf("== QScript Classical Backend (pseudo-LLVM IR) ==\n");
    for (size_t i = 0; i < module->function_count; i++) {
        const FunctionIr *f = &module->functions[i];
        printf("define @%s() {\n", f->name);
        for (size_t j = 0; j < f->block_count; j++) {
            const BasicBlock *b = &f->blocks[j];
            printf("  %s:\n", b->name);
            for (size_t k = 0; k < b->instruction_count; k++)
                printf("    ; nop\n");
        }
        printf("}\n");
    }
}

void backend_quantum_emit_stub(const ModuleIr *module) {
    printf("== QScript Quantum Backend (stub quantum IR) ==\n");
    for (size_t i = 0; i < module->function_count; i++)
        printf("; quantum view for function %s\n", module->functions[i].name);
}
