# QScript C prototype (qlangc)

C implementation of the QScript compiler: lexer, parser, AST, typecheck, IR, and classical/quantum backends.

## Build

**Windows (PowerShell):**  
If you don’t have `gcc` on PATH, download a portable MinGW once, then build:

```powershell
cd c-compiler
.\download-mingw.ps1   # one-time: downloads MinGW into tools\
.\build.ps1
.\qlangc.exe ..\examples\hello_world.qs
```

**Linux / macOS (make):**
```bash
cd c-compiler
make
./qlangc ../examples/hello_world.qs
```

**Windows or elsewhere (gcc directly):**
```bash
gcc -std=c11 -Wall -o qlangc.exe main.c lexer.c ast.c parser.c typecheck.c ir.c backend.c
```

**Options:** `--tokens` (dump tokens), `--llvm` (emit LLVM IR; pipe to clang to get an executable), `--qasm` (emit OpenQASM 2.0 for quantum programs).

You need a C compiler on PATH (e.g. [MinGW-w64](https://www.mingw-w64.org/) or [MSYS2](https://www.msys2.org/) for `gcc`, or Visual Studio for `cl`).
