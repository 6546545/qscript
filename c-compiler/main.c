#include "ast.h"
#include "backend.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "typecheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *token_kind_name(TokenKind k) {
    switch (k) {
        case TOK_FN: return "fn";
        case TOK_LET: return "let";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_LOOP: return "loop";
        case TOK_QUANTUM: return "quantum";
        case TOK_RETURN: return "return";
        case TOK_BREAK: return "break";
        case TOK_IDENTIFIER: return "identifier";
        case TOK_INTEGER_LITERAL: return "integer";
        case TOK_STRING_LITERAL: return "string";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LANGLE: return "<";
        case TOK_RANGLE: return ">";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_COLON: return ":";
        case TOK_SEMICOLON: return ";";
        case TOK_COMMA: return ",";
        case TOK_ARROW: return "->";
        case TOK_OPERATOR: return "operator";
        case TOK_EOF: return "eof";
        default: return "?";
    }
}

int main(int argc, char **argv) {
    int dump_tokens = 0, emit_llvm = 0, emit_qasm = 0;
    const char *path = NULL;
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        if (strcmp(argv[arg_idx], "--tokens") == 0) { dump_tokens = 1; continue; }
        if (strcmp(argv[arg_idx], "--llvm") == 0)   { emit_llvm = 1;   continue; }
        if (strcmp(argv[arg_idx], "--qasm") == 0)   { emit_qasm = 1;   continue; }
        path = argv[arg_idx];
    }
    if (!path) {
        fprintf(stderr, "usage: qlangc [--tokens] [--llvm] [--qasm] <source-file>\n");
        return 1;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: failed to open '%s'\n", path);
        return 1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: failed to seek '%s'\n", path);
        fclose(f);
        return 1;
    }
    long size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "error: failed to tell size of '%s'\n", path);
        fclose(f);
        return 1;
    }
    rewind(f);

    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "error: out of memory reading '%s'\n", path);
        fclose(f);
        return 1;
    }

    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[read] = '\0';

    Token *tokens = NULL;
    size_t token_count = 0;
    if (lex(buffer, &tokens, &token_count) != 0) {
        fprintf(stderr, "error: lex failed (e.g. unterminated string)\n");
        free(buffer);
        return 1;
    }

    if (dump_tokens) {
        printf("%zu tokens:\n", token_count);
        for (size_t i = 0; i < token_count; i++) {
            const Token *t = &tokens[i];
            if (t->value)
                printf("  %zu: %s \"%s\"\n", i, token_kind_name(t->kind), t->value);
            else
                printf("  %zu: %s\n", i, token_kind_name(t->kind));
        }
        lex_free(tokens, token_count);
        free(buffer);
        return 0;
    }

    Program *ast = parse(tokens, token_count);
    lex_free(tokens, token_count);
    tokens = NULL;
    if (!ast) {
        const char *err = parse_get_last_error();
        fprintf(stderr, "error: parse failed%s%s\n", err ? ": " : "", err ? err : "");
        free(buffer);
        return 1;
    }

    if (typecheck(ast) != 0) {
        fprintf(stderr, "error: type check failed\n");
        ast_free(ast);
        free(buffer);
        return 1;
    }

    ModuleIr *ir = lower_to_ir(ast);
    ast_free(ast);
    ast = NULL;
    if (!ir) {
        fprintf(stderr, "error: out of memory building IR\n");
        free(buffer);
        return 1;
    }

    if (emit_llvm) {
        backend_classical_emit_llvm(ir);
    } else if (emit_qasm) {
        backend_quantum_emit_qasm(ir);
    } else {
        backend_classical_emit(ir);
        backend_quantum_emit_stub(ir);
        printf("Compiled %s\n", path);
    }
    ir_free(ir);
    free(buffer);
    return 0;
}
