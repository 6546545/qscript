# QScript Language Server (LSP)

Provides diagnostics for QScript by running `qlangc` on document open/change and publishing parse and type errors.

## Setup

```bash
pip install -r requirements.txt
```

## Run standalone (stdio)

```bash
python qscript_lsp.py
```

The server communicates via stdin/stdout using the Language Server Protocol.

## Use with VS Code

The QScript VS Code extension (`ide/vscode-extension/`) automatically starts this LSP when:

1. The workspace root contains `tools/lsp/qscript_lsp.py`
2. Python is on PATH
3. `pygls` and `lsprotocol` are installed (`pip install -r requirements.txt`)

Diagnostics (errors and warnings) appear in the editor as you type. Hover, completion, jump-to-definition, signature help, and document formatting are also supported.
