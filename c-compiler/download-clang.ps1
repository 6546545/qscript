# Download Clang/LLVM for linking QScript LLVM IR to native binaries.
# Run once: .\download-clang.ps1
# Then add the bin folder to PATH, or run: $env:PATH = "...\tools\clang\bin;" + $env:PATH
#
# Alternative: winget install LLVM.LLVM  (adds clang to PATH system-wide)

$ErrorActionPreference = "Stop"
$toolsDir = Join-Path $PSScriptRoot "tools"
$clangDir = Join-Path $toolsDir "clang"
$clangExe = Join-Path $clangDir "bin\clang.exe"

if (Test-Path $clangExe) {
    Write-Host "Clang already present at $clangDir"
    Write-Host "clang: $clangExe"
    Write-Host "Add to PATH: `$env:PATH = `"$clangDir\bin;`" + `$env:PATH"
    exit 0
}

# LLVM 18.1.8 Windows x64 installer (adds to PATH when run)
# For portable use, we download the pre-built archive instead.
$url = "https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz"
$archivePath = Join-Path $toolsDir "clang-llvm.tar.xz"

Write-Host "Downloading Clang/LLVM 18.1.8 (~960 MB)..."
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null

try {
    Invoke-WebRequest -Uri $url -OutFile $archivePath -UseBasicParsing
} catch {
    Write-Error "Download failed: $_"
    Write-Host ""
    Write-Host "Alternative: Install via winget (adds to PATH):"
    Write-Host "  winget install LLVM.LLVM"
    Write-Host ""
    Write-Host "Or download the installer manually:"
    Write-Host "  https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/LLVM-18.1.8-win64.exe"
    exit 1
}

Write-Host "Extracting (this may take a few minutes)..."
$extractDir = Join-Path $toolsDir "clang-extract"
if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

# tar supports .tar.xz on Windows 10+
tar -xf $archivePath -C $extractDir
if ($LASTEXITCODE -ne 0) {
    Remove-Item $archivePath -Force -ErrorAction SilentlyContinue
    Write-Error "Extraction failed. Ensure tar is available (Windows 10+ has it), or install 7-Zip."
    Write-Host "Alternative: winget install LLVM.LLVM"
    exit 1
}

# Find clang.exe (layout: clang+llvm-18.1.8-x86_64-pc-windows-msvc/bin/clang.exe)
$clangExeFound = Get-ChildItem -Path $extractDir -Recurse -Filter "clang.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $clangExeFound) {
    Remove-Item $extractDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $archivePath -Force -ErrorAction SilentlyContinue
    Write-Error "Could not find clang.exe in extracted archive"
    exit 1
}

$topDir = $clangExeFound.Directory.Parent.FullName  # e.g. extractDir/clang+llvm-18.1.8-x86_64-pc-windows-msvc

# Move to tools/clang (we want tools/clang/bin/clang.exe)
if (Test-Path $clangDir) { Remove-Item $clangDir -Recurse -Force }
Move-Item -Path $topDir -Destination $clangDir -Force

Remove-Item $archivePath -Force -ErrorAction SilentlyContinue
Remove-Item $extractDir -Recurse -Force -ErrorAction SilentlyContinue

$clangExe = Join-Path $clangDir "bin\clang.exe"
if (Test-Path $clangExe) {
    Write-Host "Done. Clang at: $clangDir"
    Write-Host ""
    Write-Host "Add to PATH for this session:"
    Write-Host "  `$env:PATH = `"$clangDir\bin;`" + `$env:PATH"
    Write-Host ""
    Write-Host "Then run: .\scripts\run_tests.ps1"
} else {
    Write-Error "clang.exe not found at $clangExe"
    exit 1
}
