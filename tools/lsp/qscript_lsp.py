#!/usr/bin/env python3
"""
QScript Language Server - provides diagnostics by running qlangc on document changes.
"""

import logging
import os
import re
import tempfile
from pathlib import Path
from urllib.parse import urlparse, unquote

from lsprotocol import types
from pygls.cli import start_server
from pygls.lsp.server import LanguageServer
from pygls.workspace import TextDocument


def find_qlangc(workspace_root: str | None) -> str | None:
    """Find qlangc compiler: workspace c-compiler/, or qlangc on PATH."""
    if workspace_root:
        for name in ("qlangc.exe", "qlangc"):
            p = Path(workspace_root) / "c-compiler" / name
            if p.exists():
                return str(p)
    return "qlangc.exe" if os.name == "nt" else "qlangc"


class QScriptServer(LanguageServer):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.diagnostics: dict[str, list[types.Diagnostic]] = {}

    def run_qlangc(self, document: TextDocument) -> list[types.Diagnostic]:
        """Run qlangc on document content, return diagnostics from stderr."""
        import subprocess

        workspace = self.workspace.root_uri
        workspace_path = None
        if workspace and workspace.startswith("file://"):
            parsed = urlparse(workspace)
            workspace_path = unquote(parsed.path)
            if os.name == "nt" and workspace_path.startswith("/"):
                workspace_path = workspace_path[1:]
        qlangc = find_qlangc(workspace_path)

        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".qs", delete=False, encoding="utf-8"
        ) as f:
            f.write(document.source)
            tmp_path = f.name

        try:
            result = subprocess.run(
                [qlangc, tmp_path],
                capture_output=True,
                text=True,
                timeout=5,
                cwd=workspace_path or os.getcwd(),
            )
        except (FileNotFoundError, subprocess.TimeoutExpired) as e:
            return [
                types.Diagnostic(
                    range=types.Range(
                        start=types.Position(line=0, character=0),
                        end=types.Position(line=0, character=1),
                    ),
                    message=f"qlangc not found or timed out: {e}",
                    severity=types.DiagnosticSeverity.Warning,
                )
            ]
        finally:
            try:
                os.unlink(tmp_path)
            except OSError:
                pass

        diagnostics: list[types.Diagnostic] = []
        stderr = result.stderr or ""

        # Parse "line N: error: msg" (N 1-based from compiler; LSP uses 0-based)
        for match in re.finditer(r"line\s+(\d+):\s*error:\s*(.+?)(?:\n|$)", stderr):
            line_num = int(match.group(1))  # 1-based from compiler
            msg = match.group(2).strip()
            line_idx = max(0, line_num - 1)  # convert to 0-based
            line_count = len(document.lines) if document.lines else 1
            line_idx = min(line_idx, line_count - 1)
            line_len = len(document.lines[line_idx]) if document.lines and line_idx < len(document.lines) else 1
            diagnostics.append(
                types.Diagnostic(
                    range=types.Range(
                        start=types.Position(line=line_idx, character=0),
                        end=types.Position(line=line_idx, character=max(1, line_len)),
                    ),
                    message=msg,
                    severity=types.DiagnosticSeverity.Error,
                )
            )

        # Fallback: old format "error: parse failed: msg" or "error: type check failed: msg"
        if not diagnostics:
            match = re.search(r"error:\s*(.+?)(?:\n|$)", stderr)
            if match:
                msg = match.group(1).strip()
                line_count = len(document.lines) if document.lines else 1
                last_line = min(0, line_count - 1)
                last_char = len(document.lines[last_line]) if document.lines else 1
                diagnostics.append(
                    types.Diagnostic(
                        range=types.Range(
                            start=types.Position(line=0, character=0),
                            end=types.Position(line=last_line, character=last_char),
                        ),
                        message=msg,
                        severity=types.DiagnosticSeverity.Error,
                    )
                )

        return diagnostics

    def get_word_at_position(self, document: TextDocument, position: types.Position) -> str | None:
        """Get the identifier or keyword at the given position."""
        if position.line >= len(document.lines):
            return None
        line = document.lines[position.line]
        start = position.character
        while start > 0 and (line[start - 1].isalnum() or line[start - 1] in "_<>"):
            start -= 1
        end = position.character
        while end < len(line) and (line[end].isalnum() or line[end] in "_<>"):
            end += 1
        return line[start:end] if start < end else None

    def find_definition(self, document: TextDocument, word: str) -> types.Location | None:
        """Find definition of word (fn or let) in document."""
        for i, line in enumerate(document.lines):
            fn_match = re.match(rf"\bfn\s+{re.escape(word)}\s*\(", line)
            if fn_match:
                return types.Location(
                    uri=document.uri,
                    range=types.Range(
                        start=types.Position(line=i, character=fn_match.start(0) + 3),
                        end=types.Position(line=i, character=fn_match.start(0) + 3 + len(word)),
                    ),
                )
            let_match = re.match(rf"\blet\s+{re.escape(word)}\s*[:=]", line)
            if let_match:
                return types.Location(
                    uri=document.uri,
                    range=types.Range(
                        start=types.Position(line=i, character=let_match.start(0) + 4),
                        end=types.Position(line=i, character=let_match.start(0) + 4 + len(word)),
                    ),
                )
        return None

    def update_diagnostics(self, document: TextDocument) -> None:
        if not document.uri.endswith(".qs"):
            return
        diagnostics = self.run_qlangc(document)
        self.diagnostics[document.uri] = diagnostics
        self.text_document_publish_diagnostics(
            types.PublishDiagnosticsParams(
                uri=document.uri,
                version=document.version,
                diagnostics=diagnostics,
            )
        )


server = QScriptServer("qscript-lsp", "v0.1")


@server.feature(types.TEXT_DOCUMENT_DID_OPEN)
def did_open(ls: QScriptServer, params: types.DidOpenTextDocumentParams) -> None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if doc:
        ls.update_diagnostics(doc)


@server.feature(types.TEXT_DOCUMENT_DID_CHANGE)
def did_change(ls: QScriptServer, params: types.DidChangeTextDocumentParams) -> None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if doc:
        ls.update_diagnostics(doc)


# Hover info for keywords and built-ins
HOVER_DOCS: dict[str, str] = {
    "fn": "Function declaration: `fn name(params) -> return_type { ... }`",
    "let": "Variable binding: `let x: i32 = expr;` or `let x = expr;`",
    "if": "Conditional: `if (cond) { ... } else { ... }`",
    "else": "Else branch of if",
    "loop": "Infinite loop: `loop { ... break; }`",
    "for": "For loop: `for (init; cond; step) { ... }`",
    "while": "While loop: `while (cond) { ... }`",
    "break": "Exit loop",
    "continue": "Skip to next loop iteration",
    "return": "Return from function: `return;` or `return expr;`",
    "quantum": "Quantum block: `quantum { ... }`",
    "type": "Type alias: `type Name = i32;`",
    "true": "Boolean literal (i32 1)",
    "false": "Boolean literal (i32 0)",
    "print": "Built-in: print string to stdout",
    "print_int": "Built-in: print i32 to stdout",
    "print_bits": "Built-in: print measurement bits",
    "alloc_qreg<2>": "Allocate 2-qubit register",
    "h": "Hadamard gate",
    "cx": "CNOT gate",
    "measure_all": "Measure all qubits",
    "result": "Measurement result",
    "unit": "Unit type (no value)",
    "i32": "32-bit integer type",
    "bool": "Boolean type (i32)",
    "main": "Entry point: `fn main() -> unit`",
}


@server.feature(types.TEXT_DOCUMENT_HOVER)
def hover(ls: QScriptServer, params: types.HoverParams) -> types.Hover | None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if not doc:
        return None
    word = ls.get_word_at_position(doc, params.position)
    if not word:
        return None
    content = HOVER_DOCS.get(word)
    if not content:
        loc = ls.find_definition(doc, word)
        if loc:
            content = f"Defined in this file"
    if content:
        return types.Hover(
            contents=types.MarkupContent(kind=types.MarkupKind.Markdown, value=content),
            range=types.Range(
                start=params.position,
                end=types.Position(
                    line=params.position.line,
                    character=params.position.character + len(word or ""),
                ),
            ),
        )
    return None


# Completion items: keywords, types, built-ins
COMPLETION_ITEMS: list[tuple[str, str, types.CompletionItemKind]] = [
    (kw, HOVER_DOCS.get(kw, ""), types.CompletionItemKind.Keyword)
    for kw in ("fn", "let", "if", "else", "loop", "for", "while", "break", "continue", "return", "quantum", "type", "true", "false")
] + [
    ("unit", "Unit type", types.CompletionItemKind.TypeParameter),
    ("i32", "32-bit integer", types.CompletionItemKind.TypeParameter),
    ("bool", "Boolean type", types.CompletionItemKind.TypeParameter),
    ("main", "Entry point function", types.CompletionItemKind.Function),
    ("print", "Built-in: print string", types.CompletionItemKind.Function),
    ("print_int", "Built-in: print i32", types.CompletionItemKind.Function),
    ("print_bits", "Built-in: print bits", types.CompletionItemKind.Function),
    ("alloc_qreg<2>", "Allocate 2-qubit register", types.CompletionItemKind.Function),
    ("h", "Hadamard gate", types.CompletionItemKind.Function),
    ("cx", "CNOT gate", types.CompletionItemKind.Function),
    ("measure_all", "Measure all qubits", types.CompletionItemKind.Function),
    ("result", "Measurement result", types.CompletionItemKind.Variable),
]


@server.feature(types.TEXT_DOCUMENT_COMPLETION, types.CompletionOptions(trigger_characters=[".", ":"]))
def completion(ls: QScriptServer, params: types.CompletionParams) -> types.CompletionList:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    items = [
        types.CompletionItem(
            label=label,
            kind=kind,
            documentation=types.MarkupContent(kind=types.MarkupKind.Markdown, value=doc_str),
        )
        for label, doc_str, kind in COMPLETION_ITEMS
    ]
    # Add function names from document
    if doc:
        for line in doc.lines:
            m = re.match(r"\bfn\s+([a-zA-Z_][a-zA-Z0-9_]*)", line)
            if m and m.group(1) not in ("main",):
                name = m.group(1)
                if not any(c.label == name for c in items):
                    items.append(
                        types.CompletionItem(
                            label=name,
                            kind=types.CompletionItemKind.Function,
                            documentation=types.MarkupContent(
                                kind=types.MarkupKind.Markdown,
                                value=f"Function `{name}` defined in this file",
                            ),
                        )
                    )
    return types.CompletionList(is_incomplete=False, items=items)


@server.feature(types.TEXT_DOCUMENT_FORMATTING)
def format_document(
    ls: QScriptServer, params: types.DocumentFormattingParams
) -> list[types.TextEdit] | None:
    """Basic formatting: indent blocks and trim trailing whitespace."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if not doc or not doc.lines:
        return None
    indent_str = "\t" if params.options.insert_spaces is False else " " * (params.options.tab_size or 4)
    indent_level = 0
    result: list[str] = []
    for line in doc.lines:
        stripped = line.rstrip()
        if not stripped:
            result.append("")
            continue
        # Decrease indent for closing braces
        if stripped.startswith("}"):
            indent_level = max(0, indent_level - 1)
        new_line = indent_str * indent_level + stripped.lstrip()
        result.append(new_line)
        # Increase indent after opening brace (at end of line)
        if stripped.endswith("{"):
            indent_level += 1
    full_text = "\n".join(result) + ("\n" if doc.source.endswith("\n") else "")
    return [
        types.TextEdit(
            range=types.Range(
                start=types.Position(line=0, character=0),
                end=types.Position(line=len(doc.lines) - 1, character=len(doc.lines[-1])),
            ),
            new_text=full_text,
        )
    ]


# Signature help for built-ins
SIGNATURE_HELP: dict[str, tuple[str, list[types.ParameterInformation]]] = {
    "print": ("Print string to stdout", [types.ParameterInformation(label="s: string", documentation="String to print")]),
    "print_int": ("Print i32 to stdout", [types.ParameterInformation(label="x: i32", documentation="Integer to print")]),
    "print_bits": ("Print measurement bits", [types.ParameterInformation(label="result", documentation="Measurement result")]),
    "h": ("Hadamard gate", [types.ParameterInformation(label="q", documentation="Qubit (e.g. q[0])")]),
    "cx": ("CNOT gate", [
        types.ParameterInformation(label="ctrl", documentation="Control qubit"),
        types.ParameterInformation(label="target", documentation="Target qubit"),
    ]),
    "measure_all": ("Measure all qubits", [types.ParameterInformation(label="q: qreg", documentation="Quantum register")]),
    "alloc_qreg<2>": ("Allocate 2-qubit register", []),
    "main": ("Entry point", []),
}


def get_call_info(document: TextDocument, position: types.Position) -> tuple[str, int] | None:
    """Return (function_name, active_param_index) if cursor is inside a function call."""
    if position.line >= len(document.lines):
        return None
    line = document.lines[position.line][: position.character]
    # Find last unclosed '('
    depth = 0
    last_paren = -1
    for i in range(len(line) - 1, -1, -1):
        c = line[i]
        if c == ")":
            depth += 1
        elif c == "(":
            if depth == 0:
                last_paren = i
                break
            depth -= 1
    if last_paren < 0:
        return None
    # Get identifier before '(' (may include alloc_qreg<2>)
    before = line[:last_paren].rstrip()
    m = re.search(r"([a-zA-Z_][a-zA-Z0-9_]*(?:<[0-9]+>)?)\s*$", before)
    if not m:
        return None
    name = m.group(1)
    # Count commas in the argument part to get active parameter
    args_part = line[last_paren + 1 :]
    active = args_part.count(",")
    return (name, active)


@server.feature(
    types.TEXT_DOCUMENT_SIGNATURE_HELP,
    types.SignatureHelpOptions(trigger_characters=["(", ","]),
)
def signature_help(
    ls: QScriptServer, params: types.SignatureHelpParams
) -> types.SignatureHelp | None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if not doc:
        return None
    call_info = get_call_info(doc, params.position)
    if not call_info:
        return None
    name, active_param = call_info
    # Built-in
    if name in SIGNATURE_HELP:
        doc_str, params_list = SIGNATURE_HELP[name]
        param_labels = [p.label for p in params_list]
        sig_label = f"{name}({', '.join(param_labels)})"
        return types.SignatureHelp(
            signatures=[
                types.SignatureInformation(
                    label=sig_label,
                    documentation=types.MarkupContent(kind=types.MarkupKind.Markdown, value=doc_str),
                    parameters=params_list,
                )
            ],
            active_signature=0,
            active_parameter=min(active_param, max(0, len(params_list) - 1)) if params_list else 0,
        )
    # User-defined: parse fn name(params) -> ret
    for line in doc.lines:
        m = re.match(rf"\bfn\s+{re.escape(name)}\s*\(([^)]*)\)\s*(?:->\s*(\S+))?", line)
        if m:
            params_str = m.group(1).strip()
            ret = m.group(2) or "unit"
            param_names = [p.strip().split(":")[0] for p in params_str.split(",") if p.strip()]
            param_infos = [types.ParameterInformation(label=p) for p in param_names]
            sig_label = f"{name}({params_str}) -> {ret}"
            return types.SignatureHelp(
                signatures=[
                    types.SignatureInformation(
                        label=sig_label,
                        parameters=param_infos,
                    )
                ],
                active_signature=0,
                active_parameter=min(active_param, max(0, len(param_infos) - 1)) if param_infos else 0,
            )
    return None


@server.feature(types.TEXT_DOCUMENT_DEFINITION)
def definition(ls: QScriptServer, params: types.DefinitionParams) -> list[types.Location] | None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if not doc:
        return None
    word = ls.get_word_at_position(doc, params.position)
    if not word:
        return None
    loc = ls.find_definition(doc, word)
    return [loc] if loc else None


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    start_server(server)
