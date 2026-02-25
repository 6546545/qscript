#include "backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void escape_llvm_string(const char *s, char *out, size_t out_size) {
    size_t j = 0;
    for (; *s && j + 4 < out_size; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '\\') { j += (size_t)snprintf(out + j, out_size - j, "\\\\"); continue; }
        if (c == '"')  { j += (size_t)snprintf(out + j, out_size - j, "\\22"); continue; }
        if (c == '\n') { j += (size_t)snprintf(out + j, out_size - j, "\\0A"); continue; }
        if (c == '\r') { j += (size_t)snprintf(out + j, out_size - j, "\\0D"); continue; }
        if (c == '\t') { j += (size_t)snprintf(out + j, out_size - j, "\\09"); continue; }
        if (c >= 32) { out[j++] = (char)c; continue; }
        j += (size_t)snprintf(out + j, out_size - j, "\\%02X", c);
    }
    out[j] = '\0';
}

static const char *llvm_type_for(const char *ty) {
    if (!ty) return "void";
    if (strcmp(ty, "i32") == 0) return "i32";
    if (strcmp(ty, "unit") == 0) return "void";
    return "i8*";  /* string, qreg, etc. as opaque pointer for now */
}

static const char *llvm_icmp_for(const char *op) {
    if (!op) return "slt";
    if (strcmp(op, "<") == 0) return "slt";
    if (strcmp(op, ">") == 0) return "sgt";
    if (strcmp(op, "<=") == 0) return "sle";
    if (strcmp(op, ">=") == 0) return "sge";
    if (strcmp(op, "==") == 0) return "eq";
    if (strcmp(op, "!=") == 0) return "ne";
    return "slt";
}

static int is_param(const FunctionIr *fir, const char *name) {
    for (size_t i = 0; i < fir->param_count; i++) {
        if (fir->params[i].name && strcmp(fir->params[i].name, name) == 0) return 1;
    }
    return 0;
}

static int is_local(const FunctionIr *fir, const char *name) {
    for (size_t i = 0; i < fir->local_count; i++) {
        if (fir->locals[i] && strcmp(fir->locals[i], name) == 0) return 1;
    }
    return 0;
}

static int is_literal(const char *val) {
    if (!val || !*val) return 0;
    if (*val == '-' && val[1]) val++;
    for (; *val; val++) {
        if (*val < '0' || *val > '9') return 0;
    }
    return 1;
}

/* Emit i32 operand: literal, param (%name), local (emit load), or SSA (%binop.N). Writes to buf. */
static void emit_i32_operand(FILE *out, const FunctionIr *fir, const IrValue *v,
                             char *buf, size_t buf_size, size_t *load_counter) {
    const char *val = v->value ? v->value : "0";
    if (v->kind == IR_VAL_STR) {
        snprintf(buf, buf_size, "0");
        return;
    }
    if (v->kind == IR_VAL_SSA && val) {
        snprintf(buf, buf_size, "%%%s", val);
        return;
    }
    if (is_literal(val)) {
        snprintf(buf, buf_size, "%s", val);
        return;
    }
    if (is_param(fir, val)) {
        /* Params are stored in %name.addr; load from there */
        fprintf(out, "  %%load.%lu = load i32, i32* %%%s.addr\n", (unsigned long)(*load_counter), val);
        snprintf(buf, buf_size, "%%load.%lu", (unsigned long)(*load_counter));
        (*load_counter)++;
        return;
    }
    if (is_local(fir, val)) {
        fprintf(out, "  %%load.%lu = load i32, i32* %%%s\n", (unsigned long)(*load_counter), val);
        snprintf(buf, buf_size, "%%load.%lu", (unsigned long)(*load_counter));
        (*load_counter)++;
        return;
    }
    snprintf(buf, buf_size, "0");
}

void backend_classical_emit(const ModuleIr *module) {
    printf("== QScript Classical Backend (pseudo-LLVM IR) ==\n");
    for (size_t i = 0; i < module->function_count; i++) {
        const FunctionIr *f = &module->functions[i];
        printf("define @%s(", f->name);
        for (size_t p = 0; p < f->param_count; p++) {
            if (p > 0) printf(", ");
            printf("%s %%%s", llvm_type_for(f->params[p].type), f->params[p].name);
        }
        printf(") {\n");
        for (size_t j = 0; j < f->block_count; j++) {
            const BasicBlock *b = &f->blocks[j];
            printf("  %s:\n", b->name);
            for (size_t k = 0; k < b->instruction_count; k++) {
                const Instruction *inst = &b->instructions[k];
                if (inst->kind == IR_NOP) printf("    ; nop\n");
                else if (inst->kind == IR_CALL) printf("    ; call %s\n", inst->callee);
                else if (inst->kind == IR_RET) printf("    ; ret\n");
                else if (inst->kind == IR_COND_BR) printf("    ; cond_br %s -> %s, %s\n", inst->callee, inst->block_target, inst->block_target_else);
                else if (inst->kind == IR_BR) printf("    ; br %s\n", inst->block_target);
                else if (inst->kind == IR_ALLOCA) printf("    ; alloca %s\n", inst->callee);
                else if (inst->kind == IR_STORE) printf("    ; store %s\n", inst->callee);
                else if (inst->kind == IR_ADD) printf("    ; add %s\n", inst->callee);
            }
        }
        printf("}\n");
    }
}

static FILE *emit_out;

static void emit_llvm_impl(const ModuleIr *module) {
    FILE *out = emit_out ? emit_out : stdout;
    fprintf(out, "target triple = \"x86_64-unknown-unknown\"\n");
    fprintf(out, "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n");
    fprintf(out, "declare i32 @printf(i8*, ...)\n");
    fprintf(out, "@.str.fmt = private unnamed_addr constant [4 x i8] c\"%%s\\0A\\00\", align 1\n");

    /* Collect string constants from all calls (print and user functions) */
    size_t str_cap = 8, str_count = 0;
    char **str_consts = (char **)malloc(str_cap * sizeof(char *));
    if (!str_consts) return;
    for (size_t i = 0; i < module->function_count; i++) {
        const FunctionIr *fir = &module->functions[i];
        for (size_t j = 0; j < fir->block_count; j++) {
            const BasicBlock *b = &fir->blocks[j];
            for (size_t k = 0; k < b->instruction_count; k++) {
                const Instruction *inst = &b->instructions[k];
                if (inst->kind == IR_CALL && inst->args) {
                    for (size_t a = 0; a < inst->arg_count; a++) {
                        if (inst->args[a].kind == IR_VAL_STR && inst->args[a].value) {
                            if (str_count >= str_cap) {
                                str_cap *= 2;
                                char **n = (char **)realloc(str_consts, str_cap * sizeof(char *));
                                if (!n) { free(str_consts); return; }
                                str_consts = n;
                            }
                            str_consts[str_count++] = inst->args[a].value;
                        }
                    }
                }
            }
        }
    }

    char escaped[4096];
    for (size_t i = 0; i < str_count; i++) {
        escape_llvm_string(str_consts[i], escaped, sizeof(escaped));
        size_t len = strlen(str_consts[i]) + 1;
        fprintf(out, "@.str.%zu = private unnamed_addr constant [%zu x i8] c\"%s\\00\", align 1\n", i, len, escaped);
    }
    free(str_consts);

    size_t str_idx = 0;
    for (size_t i = 0; i < module->function_count; i++) {
        const FunctionIr *fir = &module->functions[i];
        /* main() emits i32 for C linking; others use declared return type */
        const char *ret_ty = (strcmp(fir->name, "main") == 0) ? "i32" : llvm_type_for(fir->return_type);
        fprintf(out, "define %s @%s(", ret_ty, fir->name);
        for (size_t p = 0; p < fir->param_count; p++) {
            if (p > 0) fprintf(out, ", ");
            fprintf(out, "%s %%%s", llvm_type_for(fir->params[p].type), fir->params[p].name);
        }
        fprintf(out, ") {\n");
        size_t icmp_counter = 0;
        size_t binop_counter = 0;
        size_t load_counter = 0;
        for (size_t j = 0; j < fir->block_count; j++) {
            const BasicBlock *b = &fir->blocks[j];
            fprintf(out, "%s:\n", b->name);
            /* Emit allocas for params (entry block only) so they can be assigned */
            if (j == 0 && fir->param_count > 0) {
                for (size_t p = 0; p < fir->param_count; p++) {
                    if (fir->params[p].name)
                        fprintf(out, "  %%%s.addr = alloca i32\n  store i32 %%%s, i32* %%%s.addr\n",
                                fir->params[p].name, fir->params[p].name, fir->params[p].name);
                }
            }
            /* Emit all allocas first for each block */
            for (size_t k = 0; k < b->instruction_count; k++) {
                if (b->instructions[k].kind == IR_ALLOCA && b->instructions[k].callee) {
                    fprintf(out, "  %%%s = alloca i32\n", b->instructions[k].callee);
                }
            }
            for (size_t k = 0; k < b->instruction_count; k++) {
                const Instruction *inst = &b->instructions[k];
                if (inst->kind == IR_ALLOCA) continue;
                if (inst->kind == IR_ADD && inst->callee && inst->arg_count >= 2) {
                    char left_buf[64], right_buf[64];
                    emit_i32_operand(out, fir, &inst->args[0], left_buf, sizeof(left_buf), &load_counter);
                    emit_i32_operand(out, fir, &inst->args[1], right_buf, sizeof(right_buf), &load_counter);
                    const char *llvm_op = (inst->callee[0] == '+') ? "add" : (inst->callee[0] == '-') ? "sub" :
                        (inst->callee[0] == '*') ? "mul" : (inst->callee[0] == '/') ? "sdiv" : "srem";
                    fprintf(out, "  %%binop.%lu = %s i32 %s, %s\n", (unsigned long)binop_counter, llvm_op, left_buf, right_buf);
                    binop_counter++;
                    continue;
                }
                if (inst->kind == IR_STORE && inst->callee && inst->arg_count >= 1) {
                    char val_buf[64];
                    emit_i32_operand(out, fir, &inst->args[0], val_buf, sizeof(val_buf), &load_counter);
                    /* Params use .addr; locals use name directly */
                    if (is_param(fir, inst->callee))
                        fprintf(out, "  store i32 %s, i32* %%%s.addr\n", val_buf, inst->callee);
                    else
                        fprintf(out, "  store i32 %s, i32* %%%s\n", val_buf, inst->callee);
                    continue;
                }
                if (inst->kind == IR_RET) {
                    if (strcmp(ret_ty, "i32") == 0) {
                        if (inst->arg_count >= 1 && inst->args) {
                            char val_buf[64];
                            emit_i32_operand(out, fir, &inst->args[0], val_buf, sizeof(val_buf), &load_counter);
                            fprintf(out, "  ret i32 %s\n", val_buf);
                        } else {
                            fprintf(out, "  ret i32 0\n");
                        }
                    } else {
                        fprintf(out, "  ret void\n");
                    }
                } else if (inst->kind == IR_COND_BR && inst->callee && inst->arg_count >= 2
                           && inst->block_target && inst->block_target_else) {
                    const char *icmp = llvm_icmp_for(inst->callee);
                    char left_buf[64], right_buf[64];
                    emit_i32_operand(out, fir, &inst->args[0], left_buf, sizeof(left_buf), &load_counter);
                    emit_i32_operand(out, fir, &inst->args[1], right_buf, sizeof(right_buf), &load_counter);
                    fprintf(out, "  %%cond.%zu = icmp %s i32 %s, %s\n", icmp_counter, icmp, left_buf, right_buf);
                    fprintf(out, "  br i1 %%cond.%zu, label %%%s, label %%%s\n", icmp_counter++,
                            inst->block_target, inst->block_target_else);
                } else if (inst->kind == IR_BR && inst->block_target) {
                    fprintf(out, "  br label %%%s\n", inst->block_target);
                } else if (inst->kind == IR_CALL && inst->callee && strcmp(inst->callee, "print") == 0
                           && inst->arg_count == 1 && inst->args[0].kind == IR_VAL_STR) {
                    size_t idx = str_idx++;
                    size_t n = strlen(inst->args[0].value) + 1;
                    fprintf(out, "  %%str.ptr = getelementptr inbounds ([%zu x i8], [%zu x i8]* @.str.%zu, i64 0, i64 0)\n", n, n, idx);
                    fprintf(out, "  %%fmt.ptr = getelementptr inbounds ([4 x i8], [4 x i8]* @.str.fmt, i64 0, i64 0)\n");
                    fprintf(out, "  %%1 = call i32 @printf(i8* %%fmt.ptr, i8* %%str.ptr)\n");
                } else if (inst->kind == IR_CALL && inst->callee && strcmp(inst->callee, "print") != 0) {
                    int is_in_module = 0;
                    size_t callee_fi = 0;
                    for (size_t fi = 0; fi < module->function_count; fi++) {
                        if (strcmp(module->functions[fi].name, inst->callee) == 0) {
                            is_in_module = 1;
                            callee_fi = fi;
                            break;
                        }
                    }
                    if (is_in_module) {
                        const char *callee_ret = llvm_type_for(module->functions[callee_fi].return_type);
                        if (strcmp(callee_ret, "void") == 0) {
                            fprintf(out, "  call void @%s(", inst->callee);
                        } else {
                            if (inst->result_ssa)
                                fprintf(out, "  %%%s = call %s @%s(", inst->result_ssa, callee_ret, inst->callee);
                            else
                                fprintf(out, "  call %s @%s(", callee_ret, inst->callee);
                        }
                        for (size_t a = 0; a < inst->arg_count; a++) {
                            if (a > 0) fprintf(out, ", ");
                            if (inst->args[a].kind == IR_VAL_STR && inst->args[a].value) {
                                size_t n = strlen(inst->args[a].value) + 1;
                                fprintf(out, "i8* getelementptr inbounds ([%zu x i8], [%zu x i8]* @.str.%zu, i64 0, i64 0)", n, n, str_idx++);
                            } else if (inst->args[a].kind == IR_VAL_INT) {
                                char arg_buf[64];
                                emit_i32_operand(out, fir, &inst->args[a], arg_buf, sizeof(arg_buf), &load_counter);
                                fprintf(out, "i32 %s", arg_buf);
                            } else {
                                fprintf(out, "i32 0");
                            }
                        }
                        fprintf(out, ")\n");
                    }
                }
            }
        }
        fprintf(out, "}\n");
    }
}

void backend_classical_emit_llvm(const ModuleIr *module) {
    emit_out = NULL;
    emit_llvm_impl(module);
}

void backend_classical_emit_llvm_file(const ModuleIr *module, FILE *out) {
    emit_out = out;
    emit_llvm_impl(module);
    emit_out = NULL;
}

void backend_quantum_emit_stub(const ModuleIr *module) {
    printf("== QScript Quantum Backend (stub quantum IR) ==\n");
    for (size_t i = 0; i < module->function_count; i++)
        printf("; quantum view for function %s\n", module->functions[i].name);
}

static void emit_qasm_impl(const ModuleIr *module, FILE *out) {
    FILE *f = out ? out : stdout;
    /* Find first function with quantum ops (e.g. bell_pair) */
    const FunctionIr *qfn = NULL;
    for (size_t i = 0; i < module->function_count; i++) {
        if (module->functions[i].quantum_ops && module->functions[i].quantum_op_count > 0) {
            qfn = &module->functions[i];
            break;
        }
    }
    fprintf(f, "OPENQASM 2.0;\n");
    fprintf(f, "include \"qelib1.inc\";\n");
    if (!qfn) {
        fprintf(f, "qreg q[1];\n");
        fprintf(f, "creg c[1];\n");
        return;
    }
    /* Emit from quantum IR: collect allocs, then gates, then measure */
    size_t total_qubits = 0;
    for (size_t k = 0; k < qfn->quantum_op_count; k++) {
        const QuantumOp *qop = &qfn->quantum_ops[k];
        if (qop->kind == QOP_ALLOC_QREG)
            total_qubits += qop->reg_size;
    }
    if (total_qubits == 0) total_qubits = 1;
    fprintf(f, "qreg q[%zu];\n", total_qubits);
    fprintf(f, "creg c[%zu];\n", total_qubits);
    /* Map register names to QASM qubit indices (simple: first alloc gets 0..n-1, etc.) */
    size_t next_idx = 0;
    for (size_t k = 0; k < qfn->quantum_op_count; k++) {
        const QuantumOp *qop = &qfn->quantum_ops[k];
        if (qop->kind == QOP_H && qop->reg_name) {
            fprintf(f, "h q[%zu];\n", qop->index);
        } else if (qop->kind == QOP_CX && qop->reg_name && qop->target_reg) {
            fprintf(f, "cx q[%zu],q[%zu];\n", qop->ctrl_idx, qop->target_idx);
        }
    }
    /* Measure all */
    fprintf(f, "measure q -> c;\n");
}

void backend_quantum_emit_qasm(const ModuleIr *module) {
    emit_qasm_impl(module, NULL);
}

void backend_quantum_emit_qasm_file(const ModuleIr *module, FILE *out) {
    emit_qasm_impl(module, out);
}
