#!/usr/bin/env bash
# Quick sanity test: build compiler and run hello_world
# Run from project root: ./scripts/run_tests.sh

set -e
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root/c-compiler"

# Build
if command -v make &>/dev/null; then
    echo "Building with make..."
    make
else
    echo "Building with gcc..."
    gcc -std=c11 -Wall -o qlangc main.c lexer.c ast.c parser.c typecheck.c ir.c backend.c
fi

# Compile hello_world
echo "Compiling hello_world.qs..."
./qlangc -o hello "$root/examples/hello_world.qs"

# Run and check output
echo "Running ./hello..."
out=$(./hello 2>&1)
if echo "$out" | grep -q "Hello"; then
    echo "OK: build and hello_world passed."
    exit 0
else
    echo "Expected 'Hello' in output, got: $out"
    exit 1
fi
