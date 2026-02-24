#ifndef QSCRIPT_BACKEND_H
#define QSCRIPT_BACKEND_H

#include "ir.h"
#include <stdio.h>

void backend_classical_emit(const ModuleIr *module);
void backend_classical_emit_llvm(const ModuleIr *module);
void backend_classical_emit_llvm_file(const ModuleIr *module, FILE *out);
void backend_quantum_emit_stub(const ModuleIr *module);
void backend_quantum_emit_qasm(const ModuleIr *module);
void backend_quantum_emit_qasm_file(const ModuleIr *module, FILE *out);

#endif /* QSCRIPT_BACKEND_H */
