# Quick sanity test: build compiler and run hello_world
# Run from project root: .\scripts\run_tests.ps1

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $PSCommandPath)
$compilerDir = Join-Path $root "c-compiler"
$helloQs = Join-Path $root "examples\hello_world.qs"
$helloExe = Join-Path $compilerDir "hello.exe"

# Build
Push-Location $compilerDir
try {
    if (Test-Path "Makefile") {
        $make = Get-Command make -ErrorAction SilentlyContinue
        if ($make) {
            Write-Host "Building with make..."
            & make
            if ($LASTEXITCODE -ne 0) { throw "make failed" }
        } else {
            Write-Host "Building with build.ps1..."
            & .\build.ps1
            if ($LASTEXITCODE -ne 0) { throw "build.ps1 failed" }
        }
    } else {
        & .\build.ps1
        if ($LASTEXITCODE -ne 0) { throw "build.ps1 failed" }
    }
} finally {
    Pop-Location
}

$qlangc = Join-Path $compilerDir "qlangc.exe"
if (-not (Test-Path $qlangc)) {
    $qlangc = Join-Path $compilerDir "qlangc"
}
if (-not (Test-Path $qlangc)) {
    Write-Error "qlangc not found after build"
    exit 1
}

# Compile hello_world
Write-Host "Compiling hello_world.qs..."
& $qlangc -o $helloExe $helloQs
if ($LASTEXITCODE -ne 0) {
    Write-Error "Compilation failed"
    exit 1
}

# Run and check output
Write-Host "Running hello.exe..."
$out = & $helloExe 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Error "hello.exe failed with exit code $LASTEXITCODE"
    exit 1
}
if ($out -notmatch "Hello") {
    Write-Error "Expected 'Hello' in output, got: $out"
    exit 1
}

Write-Host "OK: build and hello_world passed."
exit 0
