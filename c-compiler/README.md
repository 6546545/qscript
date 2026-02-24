# QScript C prototype (qlangc)

Minimal C implementation that reads a QScript source file and runs the lexer.

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
gcc -std=c11 -Wall -o qlangc.exe main.c lexer.c
```

You need a C compiler on PATH (e.g. [MinGW-w64](https://www.mingw-w64.org/) or [MSYS2](https://www.msys2.org/) for `gcc`, or Visual Studio for `cl`).
