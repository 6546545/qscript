#ifndef QSCRIPT_BACKEND_H
#define QSCRIPT_BACKEND_H

#include "ir.h"

void backend_classical_emit(const ModuleIr *module);
void backend_classical_emit_llvm(const ModuleIr *module);
void backend_quantum_emit_stub(const ModuleIr *module);
void backend_quantum_emit_qasm(const ModuleIr *module);

#endif /* QSCRIPT_BACKEND_H */
