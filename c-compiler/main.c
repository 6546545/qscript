#include "ast.h"
#include "backend.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "typecheck.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#define popen _popen
#define pclose _pclose
#endif

static const char *token_kind_name(TokenKind k) {
    switch (k) {
        case TOK_FN: return "fn";
        case TOK_LET: return "let";
        case TOK_IF: return "if";
        case TOK_ELSE: return "else";
        case TOK_LOOP: return "loop";
        case TOK_FOR: return "for";
        case TOK_WHILE: return "while";
        case TOK_QUANTUM: return "quantum";
        case TOK_RETURN: return "return";
        case TOK_BREAK: return "break";
        case TOK_CONTINUE: return "continue";
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
        case TOK_TRUE: return "true";
        case TOK_FALSE: return "false";
        case TOK_TYPE: return "type";
        default: return "?";
    }
}

int main(int argc, char **argv) {
    int dump_tokens = 0, emit_llvm = 0, emit_qasm = 0;
    const char *path = NULL;
    const char *out_path = NULL;
    for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
        if (strcmp(argv[arg_idx], "--tokens") == 0) { dump_tokens = 1; continue; }
        if (strcmp(argv[arg_idx], "--llvm") == 0)   { emit_llvm = 1;   continue; }
        if (strcmp(argv[arg_idx], "--qasm") == 0 || strcmp(argv[arg_idx], "--emit-qasm") == 0)
            { emit_qasm = 1; continue; }
        if (strcmp(argv[arg_idx], "-o") == 0 && arg_idx + 1 < argc) {
            out_path = argv[++arg_idx];
            continue;
        }
        path = argv[arg_idx];
    }
    if (!path) {
        fprintf(stderr, "usage: qlangc [--tokens] [--llvm] [--qasm] [-o <output>] <source-file>\n");
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
        int line = parse_get_last_error_line();
        if (line > 0 && err)
            fprintf(stderr, "line %d: error: %s\n", line, err);
        else
            fprintf(stderr, "error: parse failed%s%s\n", err ? ": " : "", err ? err : "");
        free(buffer);
        return 1;
    }

    if (typecheck(ast) != 0) {
        const char *err = typecheck_get_last_error();
        fprintf(stderr, "line 0: error: %s\n", err ? err : "type check failed");
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
        if (out_path) {
            FILE *out = fopen(out_path, "w");
            if (!out) {
                fprintf(stderr, "error: cannot open output '%s'\n", out_path);
                ir_free(ir);
                free(buffer);
                return 1;
            }
            backend_classical_emit_llvm_file(ir, out);
            fclose(out);
            printf("Wrote LLVM IR to %s\n", out_path);
        } else {
            backend_classical_emit_llvm(ir);
        }
    } else if (emit_qasm) {
        if (out_path) {
            FILE *out = fopen(out_path, "w");
            if (!out) {
                fprintf(stderr, "error: cannot open output '%s'\n", out_path);
                ir_free(ir);
                free(buffer);
                return 1;
            }
            backend_quantum_emit_qasm_file(ir, out);
            fclose(out);
            printf("Wrote OpenQASM to %s\n", out_path);
        } else {
            backend_quantum_emit_qasm(ir);
        }
    } else if (out_path) {
        /* Compile to native binary: emit LLVM to temp, invoke clang */
        FILE *tmp = fopen(".qlangc_tmp.ll", "w");
        if (!tmp) {
            fprintf(stderr, "error: cannot create temp file .qlangc_tmp.ll\n");
            ir_free(ir);
            free(buffer);
            return 1;
        }
        backend_classical_emit_llvm_file(ir, tmp);
        fclose(tmp);
        {
            char cmd[512];
#ifdef _WIN32
            /* Use GNU target when MinGW is available; otherwise MSVC (needs VS) */
            snprintf(cmd, sizeof(cmd), "clang -target x86_64-pc-windows-gnu -x ir .qlangc_tmp.ll -o \"%s\" 2>&1", out_path);
#else
            snprintf(cmd, sizeof(cmd), "clang -x ir .qlangc_tmp.ll -o \"%s\" 2>&1", out_path);
#endif
            int r = system(cmd);
            remove(".qlangc_tmp.ll");
            if (r != 0) {
                fprintf(stderr, "error: clang failed. Ensure clang is on PATH.\n");
                fprintf(stderr, "  Alternatively: qlangc --llvm -o out.ll %s && clang -x ir out.ll -o %s\n", path, out_path);
                ir_free(ir);
                free(buffer);
                return 1;
            }
        }
        printf("Built %s\n", out_path);
    } else {
        backend_classical_emit(ir);
        backend_quantum_emit_stub(ir);
        printf("Compiled %s\n", path);
    }
    ir_free(ir);
    free(buffer);
    return 0;
}
