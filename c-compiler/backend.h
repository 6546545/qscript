#ifndef QSCRIPT_BACKEND_H
#define QSCRIPT_BACKEND_H

#include "ir.h"

void backend_classical_emit(const ModuleIr *module);
void backend_quantum_emit_stub(const ModuleIr *module);

#endif /* QSCRIPT_BACKEND_H */
