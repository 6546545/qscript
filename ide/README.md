# QScript IDE

Basic IDE support for QScript via a VS Code extension.

## Quick start

1. **Install VS Code** (free): https://code.visualstudio.com/

2. **Build the QScript compiler** (from project root):
   ```bash
   cd c-compiler
   make          # or .\build.ps1 on Windows
   ```

3. **Install the extension** (development mode):
   - Open VS Code
   - Open the `ide/vscode-extension` folder (or the whole QScript project)
   - Press F5 to launch Extension Development Host
   - In the new window, open a `.qs` file or the `examples/` folder

4. **Or install from .vsix** (after packaging):
   ```bash
   cd ide/vscode-extension
   npm install
   npm run compile
   npx vsce package
   # Install the generated .vsix via: Extensions > ... > Install from VSIX
   ```

## Features

- **Syntax highlighting** for QScript (keywords, strings, numbers, comments)
- **Language config**: comment toggling (`//`, `/* */`), bracket matching, snippets
- **Build**: `Ctrl+Shift+B` (or Cmd+Shift+B on Mac) to compile the current `.qs` file
- **Run**: Command palette → "QScript: Run" to run the compiled binary
- **Tasks**: "Tasks: Run Task" → "Build QScript" or "Run QScript"
- **Diagnostics**: Parse and type errors from the LSP
- **Hover**: Documentation for keywords and built-ins
- **Completion**: Keywords, types, built-ins, and function names
- **Go to definition**: Jump to `fn` or `let` definitions
- **Format document**: Basic indentation (Shift+Alt+F)
- **Signature help**: Parameter hints when typing function calls (triggered by `(` or `,`)

## Configuration

- `qscript.qlangcPath`: Path to the qlangc compiler. Leave empty to auto-detect from workspace `c-compiler/qlangc` or `c-compiler/qlangc.exe`.

## Requirements

- VS Code 1.85 or later
- QScript compiler (`qlangc`) in `c-compiler/` or on PATH
- For LSP diagnostics: Python 3 with `pygls` and `lsprotocol` (`pip install -r tools/lsp/requirements.txt`)
