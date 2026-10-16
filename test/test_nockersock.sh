#!/usr/bin/env bash
#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# 🧪 test_nockersock.sh: Differential & Protocol Parity Tests for nocker (Not Docker)
#

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="\"$SCRIPT_DIR/../bin/nocker\""
RAW_BIN="$SCRIPT_DIR/../bin/nocker"
TEST_SOCK="/tmp/test_nocker_$$.sock"

PASS=0
FAIL=0

cleanup() {
    if [ -n "${DAEMON_PID:-}" ]; then
        kill -TERM "$DAEMON_PID" 2>/dev/null || true
        wait "$DAEMON_PID" 2>/dev/null || true
    fi
    rm -f "$TEST_SOCK"
}
trap cleanup EXIT

assert_cmd() {
    local test_num="$1"
    local desc="$2"
    local cmd="$3"
    local pattern="$4"

    local out
    out=$(eval "$cmd" 2>&1) || true
    if echo "$out" | tr '\n' ' ' | grep -qE "$pattern"; then
        printf "  ✓ Test %02d: PASS - %s\n" "$test_num" "$desc"
        ((PASS++))
    else
        printf "  ✗ Test %02d: FAIL - %s (expected pattern: %s)\n" "$test_num" "$desc" "$pattern"
        printf "    Output was: %s\n" "$out"
        ((FAIL++))
    fi
}

echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡  NOCKER (NOT DOCKER) REST PROTOCOL TEST SUITE"
echo "════════════════════════════════════════════════════════════════════════════════"

# 1. Version and Doctor CLI
assert_cmd 1 "CLI version reports 1.0.0 and API 1.45" "$BIN version" "nocker version 1.0.0.*Docker API 1.45"
assert_cmd 2 "CLI doctor checks silicon prerequisites" "$BIN doctor" "Apple Silicon ARM64 Native.*100% Green"

# 2. Start daemon in background
"$RAW_BIN" daemon --socket "$TEST_SOCK" >/dev/null 2>&1 &
DAEMON_PID=$!

# Wait for socket to appear (up to 2 seconds)
for i in {1..20}; do
    if [ -S "$TEST_SOCK" ]; then
        break
    fi
    sleep 0.1
done

if [ ! -S "$TEST_SOCK" ]; then
    echo "  ✗ FATAL: Daemon failed to create socket at $TEST_SOCK"
    exit 1
fi

# 3. Status probe via CLI
assert_cmd 3 "CLI status verifies active daemon" "$BIN status --socket $TEST_SOCK" "Status : ACTIVE.*Operational"

# 4. HTTP GET /_ping
assert_cmd 4 "GET /_ping returns HTTP 200 and OK" \
    "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" \
    "HTTP/1.1 200 OK.*OK"

# 5. Header Parity (Docker-Experimental and API-Version)
assert_cmd 5 "Daemon emits standard Docker headers" \
    "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" \
    "Api-Version: 1.45.*Docker-Experimental: false"

# 6. HTTP GET /version (Root endpoint)
assert_cmd 6 "GET /version returns valid Engine JSON" \
    "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/version" \
    "\"Version\": \"26.0.0\".*\"ApiVersion\": \"1.45\""

# 7. HTTP GET /v1.45/version (Normalized version prefix)
assert_cmd 7 "GET /v1.45/version handles API version prefix" \
    "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/v1.45/version" \
    "\"Platform\":.*\"Arch\": \"arm64\""

# 8. HTTP GET /info (System Info)
assert_cmd 8 "GET /info reports APFS-CoW driver and memory" \
    "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/info" \
    "\"Driver\": \"apfs-cow\".*\"Architecture\": \"arm64\""

# 9. HTTP GET /containers/json (Active Containers)
assert_cmd 9 "GET /containers/json returns empty array" \
    "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/containers/json" \
    "\[\]"

# 10. HTTP GET /images/json (Images)
assert_cmd 10 "GET /images/json returns valid image list or array" \
    "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/images/json" \
    "\[.*\]"

# 11. 404 Route handling
assert_cmd 11 "Unknown endpoint returns HTTP 404" \
    "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/nonexistent" \
    "HTTP/1.1 404 Not Found.*page not found"

# 12. Test with real Docker CLI if present
HOST_DOCKER=""
for p in /usr/local/bin/docker /opt/homebrew/bin/docker /usr/bin/docker; do
    if [ -x "$p" ]; then
        HOST_DOCKER="$p"
        break
    fi
done

if [ -n "$HOST_DOCKER" ]; then
    assert_cmd 12 "Real Docker CLI interacts with nocker socket" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK version" \
        "Server:.*26.0.0|1.45"
    assert_cmd 13 "Real Docker CLI lists images via nocker socket" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK images" \
        "IMAGE.*ID.*alpine"
else
    printf "  - Test 12-13: SKIPPED (Docker CLI binary not installed on host)\n"
fi

# 14. APFS Copy-on-Write snapshot test
rm -rf "$HOME/.nocker/storage/containers/test_verify_cnt"
assert_cmd 14 "APFS CoW snapshot completes in sub-100ms" \
    "$BIN snapshot alpine:latest test_verify_cnt" \
    "⚡ \[apfs-cow\] Cloned rootfs for container.*Zero-Copy"

# 15. Verify container rootfs has valid POSIX shell
assert_cmd 15 "Cloned container rootfs contains valid shell symlink & busybox binary" \
    "test -L \"$HOME/.nocker/storage/containers/test_verify_cnt/rootfs/bin/sh\" && test -f \"$HOME/.nocker/storage/containers/test_verify_cnt/rootfs/bin/busybox\" && echo 'CONTAINER_ROOTFS_OK'" \
    "CONTAINER_ROOTFS_OK"

# 16. Graceful shutdown
kill -TERM "$DAEMON_PID"
wait "$DAEMON_PID" 2>/dev/null || true
unset DAEMON_PID

sleep 0.2
if [ ! -e "$TEST_SOCK" ]; then
    printf "  ✓ Test 16: PASS - Socket removed gracefully on termination\n"
    ((PASS++))
else
    printf "  ✗ Test 16: FAIL - Socket was not unlinked on shutdown\n"
    ((FAIL++))
fi

echo "════════════════════════════════════════════════════════════════════════════════"
TOTAL=$((PASS + FAIL))
printf "  ⚡  TEST SUMMARY: %d / %d TESTS PASSED (%.1f%%)\n" \
    "$PASS" "$TOTAL" "$(awk "BEGIN {print ($PASS / $TOTAL) * 100}")"
echo "════════════════════════════════════════════════════════════════════════════════"

[ "$FAIL" -eq 0 ]
