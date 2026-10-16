#!/bin/bash
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ nocker: Multi-Language AST & Assembly Parity Test Runner
# Mounts sample code into C, C++, Zig, Go, and PHP containers and displays AST/Assembly output.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LANG_DIR="${SCRIPT_DIR}/languages"

echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡  NOCKER MULTI-LANGUAGE AST & ASSEMBLY BENCHMARK RUNNER"
echo "════════════════════════════════════════════════════════════════════════════════"

run_lang_test() {
    local LANG_NAME="$1"
    local IMAGE="$2"
    local CMD="$3"

    echo ""
    echo "────────────────────────────────────────────────────────────────────────────────"
    echo "🚀 Testing Language: [ ${LANG_NAME} ] inside Image [ ${IMAGE} ]"
    echo "────────────────────────────────────────────────────────────────────────────────"
    nocker run --timeout 20000 -v "${LANG_DIR}:/workspace" "${IMAGE}" "${CMD}"
}

# 1. C Test
run_lang_test "C (Clang AST & ARM64 Assembly)" "runner-c" \
"echo '--- [1. Clang AST Excerpt] ---' && clang -Xclang -ast-dump -fsyntax-only workspace/sample.c | grep -E 'FunctionDecl.*compute_dot_product' -A 8 | head -n 10 && \
 echo '--- [2. ARM64 Assembly (madd/ldp)] ---' && clang -S -O2 -o - workspace/sample.c | grep -E '_compute_dot_product:|mul|madd|ldr' -A 6 | head -n 10 && \
 echo '--- [3. Execution Result] ---' && clang -O3 workspace/sample.c -o /tmp/c_out && /tmp/c_out"

# 2. C++ Test
run_lang_test "C++20 (Templates, Concepts & Assembly)" "runner-cpp" \
"echo '--- [1. Clang C++20 Concept AST] ---' && (clang++ -std=c++20 -Xclang -ast-dump -fsyntax-only workspace/sample.cpp 2>/dev/null || true) | awk '/ConceptDecl.*Numeric|ClassTemplateDecl/{f=1; c=0} f{print; c++; if(c>8) f=0}' && \
 echo '--- [2. ARM64 Assembly (sub/add)] ---' && clang++ -std=c++20 -S -O2 -o - workspace/sample.cpp | grep -E '_main:|add|ldr|sub' -A 6 | head -n 8 && \
 echo '--- [3. Execution Result] ---' && clang++ -std=c++20 -O3 workspace/sample.cpp -o /tmp/cpp_out && /tmp/cpp_out"

# 3. Zig Test
run_lang_test "Zig (AST Check & ARM64 Assembly)" "runner-zig" \
"echo '--- [1. Zig AST Syntax Check] ---' && zig ast-check workspace/sample.zig && echo '✓ Zig AST is valid and fully checked' && \
 echo '--- [2. Assembly Symbol Verification] ---' && zig build-exe -OReleaseFast workspace/sample.zig -femit-asm=/tmp/zig.s -femit-bin=/tmp/zig_out && grep -E 'fast_power' /tmp/zig.s | head -n 5 && \
 echo '--- [3. Execution Result] ---' && /tmp/zig_out"

# 4. Go Test
run_lang_test "Go (Plan 9 / ARM64 Assembly)" "runner-go" \
"echo '--- [1. Go Compiler ARM64 Assembly (squareWorker)] ---' && go build -gcflags='-S' workspace/sample.go 2>&1 | grep -E 'main.squareWorker|MOVD|CMP|BLS' | head -n 8 && \
 echo '--- [2. Execution Result] ---' && go run workspace/sample.go"

# 5. PHP Test
run_lang_test "PHP 8.2 (Zend VM Opcodes AST)" "php:8.2-cli" \
"echo '--- [1. Zend VM Bytecode Opcodes] ---' && php -d opcache.enable_cli=1 -d opcache.opt_debug_level=0x10000 workspace/sample.php 2>&1 | grep -E 'DECLARE_CLASS|NEW|SEND_VAL|DO_FCALL|ASSIGN' | head -n 8 && \
 echo '--- [2. Execution Result] ---' && php workspace/sample.php"

echo ""
echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡  ALL 5 LANGUAGES COMPILED & EXECUTED SUCCESSFULLY THROUGH NOCKER!"
echo "════════════════════════════════════════════════════════════════════════════════"
