# Download portable MinGW-w64 (WinLibs) for building qlangc on Windows.
# Run once: .\download-mingw.ps1
# Then: .\build.ps1  (will use .\tools\mingw\bin\gcc.exe)

$ErrorActionPreference = "Stop"
$toolsDir = Join-Path $PSScriptRoot "tools"
$mingwDir = Join-Path $toolsDir "mingw"
$binDir = Join-Path $mingwDir "bin"
$gccPath = Join-Path $binDir "gcc.exe"

if (Test-Path $gccPath) {
    Write-Host "MinGW already present at $mingwDir"
    Write-Host "gcc: $gccPath"
    exit 0
}

# WinLibs GCC 15.2.0 + MinGW-w64 13.0.0 (UCRT), Win64, ZIP
$url = "https://github.com/brechtsanders/winlibs_mingw/releases/download/15.2.0posix-13.0.0-ucrt-r6/winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-13.0.0-r6.zip"
$zipPath = Join-Path $toolsDir "winlibs-mingw.zip"

Write-Host "Downloading MinGW-w64 (WinLibs)..."
New-Item -ItemType Directory -Force -Path $toolsDir | Out-Null

try {
    Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing
} catch {
    Write-Error "Download failed: $_"
    exit 1
}

Write-Host "Extracting..."
Expand-Archive -Path $zipPath -DestinationPath $toolsDir -Force
Remove-Item $zipPath -Force

# Find bin\gcc.exe inside extracted archive (layout varies by release)
$gccExe = Get-ChildItem -Path $toolsDir -Recurse -Filter "gcc.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $gccExe) {
    Write-Error "Could not find gcc.exe inside extracted archive; check $toolsDir"
    exit 1
}
$mingwDir = $gccExe.Directory.Parent.FullName
$gccPath = $gccExe.FullName

if (Test-Path $gccPath) {
    Write-Host "Done. MinGW at: $mingwDir"
    Write-Host "Run: .\build.ps1"
} else {
    Write-Error "gcc.exe not found at $gccPath"
    exit 1
}
