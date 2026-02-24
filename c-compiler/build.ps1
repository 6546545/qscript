# Build qlangc on Windows (PowerShell)
# Usage: .\build.ps1
# If gcc is not on PATH, run .\download-mingw.ps1 first to fetch a portable MinGW.

$ErrorActionPreference = "Stop"
$src = @("main.c", "lexer.c", "ast.c", "parser.c", "typecheck.c", "ir.c", "backend.c")
$out = "qlangc.exe"

# Prefer gcc: from PATH, or from tools\ (after download-mingw.ps1)
$gccExe = $null
$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if ($gcc) { $gccExe = $gcc.Source }
if (-not $gccExe) {
    $toolsGcc = Get-ChildItem -Path (Join-Path $PSScriptRoot "tools") -Recurse -Filter "gcc.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($toolsGcc) { $gccExe = $toolsGcc.FullName }
}
if ($gccExe) {
    Write-Host "Building with gcc..."
    & $gccExe -std=c11 -Wall -o $out $src
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Built: $out"
        Write-Host "Run: .\$out ..\examples\hello_world.qs"
        exit 0
    }
}

$cl = Get-Command cl -ErrorAction SilentlyContinue
if ($cl) {
    Write-Host "Building with MSVC cl..."
    & cl /nologo /Fe:$out $src
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Built: $out"
        Write-Host "Run: .\$out ..\examples\hello_world.qs"
        exit 0
    }
}

Write-Error "No C compiler found. Install MinGW/gcc or Visual Studio (cl) and ensure it is on PATH."
exit 1
