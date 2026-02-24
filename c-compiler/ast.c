#include "ast.h"
#include <stdlib.h>

void ast_free(Program *program) {
    if (!program) return;
    for (size_t i = 0; i < program->function_count; i++)
        free(program->functions[i].name);
    free(program->functions);
    program->functions = NULL;
    program->function_count = 0;
}
