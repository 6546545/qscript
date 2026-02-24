#ifndef QSCRIPT_TYPECHECK_H
#define QSCRIPT_TYPECHECK_H

#include "ast.h"

/**
 * Type-check the program.
 * Returns 0 on success, -1 on type error.
 */
int typecheck(const Program *program);

/** Last typecheck error message (when typecheck() returned -1). */
const char *typecheck_get_last_error(void);

#endif /* QSCRIPT_TYPECHECK_H */
