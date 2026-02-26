import * as vscode from "vscode";
import * as path from "path";
import * as fs from "fs";
import {
  LanguageClient,
  LanguageClientOptions,
  ServerOptions,
} from "vscode-languageclient/node";

let outputChannel: vscode.OutputChannel;
let client: LanguageClient | undefined;

class QScriptTaskProvider implements vscode.TaskProvider {
  provideTasks(): vscode.Task[] {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "qscript") return [];
    const file = editor.document.uri.fsPath;
    const outName = process.platform === "win32" ? "out.exe" : "out";
    const outPath = path.join(path.dirname(file), outName);
    const qlangc = findQlangc();
    if (!qlangc) return [];

    const buildTask = new vscode.Task(
      { type: "qscript", file },
      vscode.TaskScope.Workspace,
      "Build QScript",
      "qscript"
    );
    buildTask.execution = new vscode.ShellExecution(qlangc, ["-o", outPath, file], {
      cwd: path.dirname(file),
    });
    buildTask.group = vscode.TaskGroup.Build;
    buildTask.presentationOptions = { reveal: "always", panel: "shared" };

    const runTask = new vscode.Task(
      { type: "qscript", file },
      vscode.TaskScope.Workspace,
      "Run QScript",
      "qscript"
    );
    const runCmd = process.platform === "win32" ? `.\\${outName}` : `./${outName}`;
    runTask.execution = new vscode.ShellExecution(runCmd, { cwd: path.dirname(outPath) });
    runTask.group = vscode.TaskGroup.Test;
    runTask.presentationOptions = { reveal: "always", panel: "shared" };

    return [buildTask, runTask];
  }

  resolveTask(task: vscode.Task): vscode.Task | undefined {
    return task;
  }
}

function getOutputChannel(): vscode.OutputChannel {
  if (!outputChannel) {
    outputChannel = vscode.window.createOutputChannel("QScript");
  }
  return outputChannel;
}

function findQlangc(): string | null {
  const configPath = vscode.workspace.getConfiguration("qscript").get<string>("qlangcPath");
  if (configPath && fs.existsSync(configPath)) {
    return configPath;
  }
  const workspaceFolders = vscode.workspace.workspaceFolders;
  if (workspaceFolders) {
    for (const folder of workspaceFolders) {
      const candidates = [
        path.join(folder.uri.fsPath, "c-compiler", "qlangc.exe"),
        path.join(folder.uri.fsPath, "c-compiler", "qlangc"),
      ];
      for (const c of candidates) {
        if (fs.existsSync(c)) return c;
      }
    }
  }
  const onPath = process.platform === "win32" ? "qlangc.exe" : "qlangc";
  return onPath;
}

function startLspClient(context: vscode.ExtensionContext): void {
  const workspaceFolders = vscode.workspace.workspaceFolders;
  if (!workspaceFolders?.length) return;

  const root = workspaceFolders[0].uri.fsPath;
  const lspPath = path.join(root, "tools", "lsp", "qscript_lsp.py");
  if (!fs.existsSync(lspPath)) return;

  const pythonCmd = process.platform === "win32" ? "python" : "python3";
  const serverOptions: ServerOptions = {
    run: {
      command: pythonCmd,
      args: [lspPath],
      options: { cwd: root },
    },
    debug: {
      command: pythonCmd,
      args: [lspPath],
      options: { cwd: root },
    },
  };

  const clientOptions: LanguageClientOptions = {
    documentSelector: [{ scheme: "file", language: "qscript" }],
    synchronize: { fileEvents: vscode.workspace.createFileSystemWatcher("**/*.qs") },
  };

  client = new LanguageClient(
    "qscript-lsp",
    "QScript Language Server",
    serverOptions,
    clientOptions
  );
  client.start().catch((err) => {
    console.warn("QScript LSP failed to start:", err);
  });
  context.subscriptions.push(
    new vscode.Disposable(() => {
      if (client) client.stop();
    })
  );
}

export function activate(context: vscode.ExtensionContext) {
  startLspClient(context);

  context.subscriptions.push(
    vscode.tasks.registerTaskProvider("qscript", new QScriptTaskProvider())
  );

  const buildCmd = vscode.commands.registerCommand("qscript.build", async () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "qscript") {
      vscode.window.showWarningMessage("Open a .qs file to build.");
      return;
    }
    const file = editor.document.uri.fsPath;
    const qlangc = findQlangc();
    if (!qlangc) {
      vscode.window.showErrorMessage(
        "qlangc not found. Set qscript.qlangcPath or add c-compiler/qlangc to your workspace."
      );
      return;
    }
    const outName = process.platform === "win32" ? "out.exe" : "out";
    const outPath = path.join(path.dirname(file), outName);
    const channel = getOutputChannel();
    channel.clear();
    channel.appendLine(`Building ${file}...`);
    const { execFile } = await import("child_process");
    const { promisify } = await import("util");
    const exec = promisify(execFile);
    try {
      const { stdout, stderr } = await exec(qlangc, ["-o", outPath, file], {
        cwd: path.dirname(file),
      });
      if (stdout) channel.append(stdout);
      if (stderr) channel.append(stderr);
      channel.appendLine("Build succeeded.");
      vscode.window.showInformationMessage("QScript build succeeded.");
    } catch (err: unknown) {
      const e = err as { stderr?: string; stdout?: string };
      if (e.stderr) channel.append(e.stderr);
      if (e.stdout) channel.append(e.stdout);
      channel.appendLine("Build failed.");
      vscode.window.showErrorMessage("QScript build failed. See Output > QScript.");
    }
    channel.show();
  });

  const runCmd = vscode.commands.registerCommand("qscript.run", async () => {
    const editor = vscode.window.activeTextEditor;
    if (!editor || editor.document.languageId !== "qscript") {
      vscode.window.showWarningMessage("Open a .qs file to run.");
      return;
    }
    const file = editor.document.uri.fsPath;
    const outName = process.platform === "win32" ? "out.exe" : "out";
    const outPath = path.join(path.dirname(file), outName);
    if (!fs.existsSync(outPath)) {
      const build = await vscode.window.showQuickPick(
        ["Build and Run", "Cancel"],
        { placeHolder: "Binary not found. Build first?" }
      );
      if (build === "Build and Run") {
        await vscode.commands.executeCommand("qscript.build");
        if (!fs.existsSync(outPath)) return;
      } else {
        return;
      }
    }
    const term = vscode.window.createTerminal({
      name: "QScript Run",
      cwd: path.dirname(outPath),
    });
    term.sendText(process.platform === "win32" ? `.\\${outName}` : `./${outName}`);
    term.show();
  });

  context.subscriptions.push(buildCmd, runCmd);
}

export function deactivate(): Thenable<void> | void {
  if (client) return client.stop();
}
