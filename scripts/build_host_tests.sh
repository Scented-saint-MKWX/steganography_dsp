#!/usr/bin/env bash
# build_host_tests.sh - Build and execute all host verification tests
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== Building Host Tests via Makefile ==="
cd "$PROJECT_ROOT/target"
make clean
make host
make host_stego

echo ""
echo "=== Running Unit Test Suite ==="
./build_host/test_runner

echo ""
echo "=== Running Host Stego Execution ==="
./build_host/stego_host

echo ""
echo "All host test binaries compiled and passed successfully!"
