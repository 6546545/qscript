#ifndef QSCRIPT_PARSER_H
#define QSCRIPT_PARSER_H

#include "ast.h"
#include "lexer.h"
#include <stddef.h>

/**
 * Parse token stream into a Program (AST).
 * Returns program on success (caller must ast_free), NULL on parse error.
 */
Program *parse(const Token *tokens, size_t token_count);

/** Last parse error message (when parse() returned NULL). */
const char *parse_get_last_error(void);

#endif /* QSCRIPT_PARSER_H */
