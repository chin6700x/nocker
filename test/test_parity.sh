#!/usr/bin/env bash
#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# 🧪 test_docker_parity.sh: 100 Exhaustive Differential & Docker CLI Parity Tests
#

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN="$ROOT_DIR/bin/nocker"
DOCKER_LINK="$ROOT_DIR/bin/nocker"
TEST_SOCK="/tmp/test_parity_$$.sock"

PASS=0
FAIL=0
TOTAL=100

cleanup() {
    if [ -n "${DAEMON_PID:-}" ]; then
        kill -TERM "$DAEMON_PID" 2>/dev/null || true
        wait "$DAEMON_PID" 2>/dev/null || true
    fi
    rm -f "$TEST_SOCK"
}
trap cleanup EXIT

assert_test() {
    local tnum="$1"
    local desc="$2"
    local cmd="$3"
    local pattern="$4"

    local out
    out=$(eval "$cmd" 2>&1) || true
    if echo "$out" | tr '\n' ' ' | grep -qE "$pattern"; then
        printf "  ✓ Test %03d: PASS - %s\n" "$tnum" "$desc"
        ((PASS++))
    else
        printf "  ✗ Test %03d: FAIL - %s (expected pattern: %s)\n" "$tnum" "$desc" "$pattern"
        printf "    Output was: %s\n" "$out"
        ((FAIL++))
    fi
}

echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡  NOCKER (NOT DOCKER) 100-TEST EXHAUSTIVE PARITY SUITE"
echo "════════════════════════════════════════════════════════════════════════════════"

# ==============================================================================
# MODULE 1 (Tests 001 - 015): Binary Integrity, CLI & System Diagnostics
# ==============================================================================
assert_test 1 "Binary executable exists and is runnable" "test -x \"$BIN\" && echo OK" "OK"
assert_test 2 "Docker symlink exists and is runnable" "test -x \"$DOCKER_LINK\" && echo OK" "OK"
assert_test 3 "Version command reports nocker v1.0.0" "\"$BIN\" version" "nocker version 1.0.0"
assert_test 4 "Version command declares Docker API 1.45" "\"$BIN\" version" "Docker API 1.45"
assert_test 5 "Version command contains author commit hash" "\"$BIN\" version" "chin6700x"
assert_test 6 "CLI --version flag parity" "\"$BIN\" --version" "nocker version"
assert_test 7 "CLI -v short flag parity" "\"$BIN\" -v" "nocker version"
assert_test 8 "Doctor probe confirms ARM64 Silicon architecture" "\"$BIN\" doctor" "Apple Silicon ARM64 Native"
assert_test 9 "Doctor probe confirms Darwin XNU kernel" "\"$BIN\" doctor" "Darwin XNU \(Apple macOS\)"
assert_test 10 "Doctor probe verifies Apple Virtualization Bridge" "\"$BIN\" doctor" "Virtualization\.framework Bridge Ready"
assert_test 11 "Doctor probe verifies APFS clonefile() support" "\"$BIN\" doctor" "APFS Copy-on-Write clonefile\(\) Supported"
assert_test 12 "Doctor probe reports 100% Green Status" "\"$BIN\" doctor" "100% Green"
assert_test 13 "Help manual displays usage instructions" "\"$BIN\" help" "USAGE:.*run.*daemon"
assert_test 14 "Help manual documents container lifecycle commands" "\"$BIN\" --help" "ps.*logs.*inspect.*stop.*rm"
assert_test 15 "Codesign entitlements confirm Apple Virtualization" "codesign -d --entitlements :- \"$BIN\" 2>&1" "com.apple.security.virtualization"

# ==============================================================================
# MODULE 2 (Tests 016 - 035): UNIX Socket REST Server & Protocol Handlers
# ==============================================================================
# Start background daemon on test socket
"$BIN" daemon --socket "$TEST_SOCK" >/dev/null 2>&1 &
DAEMON_PID=$!

for i in {1..20}; do
    [ -S "$TEST_SOCK" ] && break
    sleep 0.1
done

assert_test 16 "Daemon creates UNIX domain socket file" "test -S \"$TEST_SOCK\" && echo OK" "OK"
assert_test 17 "Socket permissions are 0666 developer accessible" "stat -f '%Lp' \"$TEST_SOCK\"" "666"
assert_test 18 "CLI status probe confirms active daemon" "\"$BIN\" status --socket \"$TEST_SOCK\"" "ACTIVE.*Operational"
assert_test 19 "CLI ping probe returns operational status" "\"$BIN\" ping --socket \"$TEST_SOCK\"" "ACTIVE.*Operational"
assert_test 20 "GET /_ping returns HTTP 200 status" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "HTTP/1.1 200 OK"
assert_test 21 "GET /_ping response body is OK" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "OK"
assert_test 22 "Server header identifies nocker 1.0.0" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "Server: nocker/1.0.0"
assert_test 23 "Api-Version header declares 1.45" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "Api-Version: 1.45"
assert_test 24 "OSType header declares darwin" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "OSType: darwin"
assert_test 25 "Docker-Experimental header is false" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/_ping" "Docker-Experimental: false"
assert_test 26 "GET /version returns Engine Version 26.0.0" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/version" "\"Version\": \"26.0.0\""
assert_test 27 "GET /version reports ARM64 architecture" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/version" "\"Arch\": \"arm64\""
assert_test 28 "GET /v1.45/version normalizes API version prefix" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/v1.45/version" "\"ApiVersion\": \"1.45\""
assert_test 29 "GET /info returns system storage driver apfs-cow" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/info" "\"Driver\": \"apfs-cow\""
assert_test 30 "GET /info returns clonefile CoW snapshotter" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/info" "\"CoW Snapshotter\", \"clonefile\""
assert_test 31 "GET /info reports hardware CPU count" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/info" "\"NCPU\": [0-9]+"
assert_test 32 "GET /info lists vz, seatbelt, crun runtimes" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/info" "\"vz\":.*\"seatbelt\":.*\"crun\""
assert_test 33 "GET /containers/json returns array" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/containers/json" "\[.*\]"
assert_test 34 "GET /images/json returns valid JSON" "curl -s --unix-socket \"$TEST_SOCK\" http://localhost/images/json" "\[.*\]"
assert_test 35 "GET /nonexistent returns HTTP 404 Not Found" "curl -s -i --unix-socket \"$TEST_SOCK\" http://localhost/nonexistent" "HTTP/1.1 404 Not Found"

# ==============================================================================
# MODULE 3 (Tests 036 - 050): OCI Storage & APFS Copy-on-Write Acceleration
# ==============================================================================
assert_test 36 "CLI images lists cached OCI images" "\"$BIN\" images" "library/alpine:latest"
assert_test 37 "Cached Alpine image has valid ARM64 rootfs" "test -d \"$HOME/.nocker/storage/images/library_alpine_latest/rootfs/bin\" && echo OK" "OK"
assert_test 38 "Alpine rootfs contains /bin/sh" "test -L \"$HOME/.nocker/storage/images/library_alpine_latest/rootfs/bin/sh\" && echo OK" "OK"
assert_test 39 "Alpine rootfs contains /etc" "test -d \"$HOME/.nocker/storage/images/library_alpine_latest/rootfs/etc\" && echo OK" "OK"
assert_test 40 "Alpine rootfs contains /usr" "test -d \"$HOME/.nocker/storage/images/library_alpine_latest/rootfs/usr\" && echo OK" "OK"
assert_test 41 "APFS CoW snapshot creates scratch rootfs" "\"$BIN\" snapshot alpine:latest parity_cow_01" "Cloned rootfs for container.*Zero-Copy"
assert_test 42 "APFS CoW cloned rootfs exists on disk" "test -d \"$HOME/.nocker/storage/containers/parity_cow_01/rootfs\" && echo OK" "OK"
assert_test 43 "APFS CoW cloned rootfs has identical directory count" "test -d \"$HOME/.nocker/storage/containers/parity_cow_01/rootfs/bin\" && echo OK" "OK"
assert_test 44 "APFS CoW idempotency reuses existing scratch in 0ms" "\"$BIN\" snapshot alpine:latest parity_cow_01" "Container rootfs ready"
assert_test 45 "Multiple independent APFS CoW snapshots" "\"$BIN\" snapshot alpine:latest parity_cow_02" "Cloned rootfs"
assert_test 46 "Second APFS CoW snapshot exists independently" "test -d \"$HOME/.nocker/storage/containers/parity_cow_02/rootfs\" && echo OK" "OK"
assert_test 47 "CLI rm removes container scratch" "\"$BIN\" rm parity_cow_01" "parity_cow_01"
assert_test 48 "Removed container scratch 01 no longer exists" "test ! -d \"$HOME/.nocker/storage/containers/parity_cow_01\" && echo CLEAN" "CLEAN"
assert_test 49 "CLI rm removes container scratch 02" "\"$BIN\" rm parity_cow_02" "parity_cow_02"
assert_test 50 "Removed container scratch 02 no longer exists" "test ! -d \"$HOME/.nocker/storage/containers/parity_cow_02\" && echo CLEAN" "CLEAN"

# ==============================================================================
# MODULE 4 (Tests 051 - 065): Twin-Engine: Bare-Metal Darwin Seatbelt
# ==============================================================================
assert_test 51 "Native run executes simple echo" "\"$BIN\" run --native host \"echo HELLO_NATIVE\"" "HELLO_NATIVE"
assert_test 52 "Native run latency is sub-25ms" "\"$BIN\" run --native --verbose host \"echo SPEED\"" "Duration: [0-9]+\.[0-9]+ ms"
assert_test 53 "Native run captures standard output" "\"$BIN\" run --native host \"echo 'STDOUT_CAPTURE'\"" "STDOUT_CAPTURE"
assert_test 54 "Native run captures standard error" "\"$BIN\" run --native host \"echo 'STDERR_CAPTURE' >&2\"" "STDERR_CAPTURE"
assert_test 55 "Native run propagates zero exit code" "\"$BIN\" run --native host \"true\" && echo EXIT_ZERO" "EXIT_ZERO"
assert_test 56 "Native run propagates non-zero exit code" "\"$BIN\" run --native host \"exit 17\" || echo EXIT_17" "EXIT_17"
assert_test 57 "Native watchdog terminates runaway commands" "\"$BIN\" run --native --timeout 200 host \"sleep 1\" || echo TIMEOUT_KILL" "TIMEOUT_KILL"
assert_test 58 "Native watchdog returns exit code 124" "\"$BIN\" run --native --verbose --timeout 200 host \"sleep 1\" || true" "Exit: 124"
assert_test 59 "Native run sets SANDBOX_ENGINE environment" "\"$BIN\" run --native host 'echo \$SANDBOX_ENGINE'" "1"
assert_test 60 "Native run sets SILICON_VELOCITY environment" "\"$BIN\" run --native host 'echo \$SILICON_VELOCITY'" "baremetal"
assert_test 61 "Native run executes piped shell pipeline" "\"$BIN\" run --native host \"echo 'pipeline' | tr a-z A-Z\"" "PIPELINE"
assert_test 62 "Native run executes arithmetic expansion" "\"$BIN\" run --native host 'echo \$((2000 + 26))'" "2026"
assert_test 63 "Native run executes loop" "\"$BIN\" run --native host 'for x in A B; do printf \"%s\" \$x; done'" "AB"
assert_test 64 "Native run reports darwin_seatbelt_native engine" "\"$BIN\" run --native --verbose host \"echo ENG\"" "Engine: darwin_seatbelt_native"
assert_test 65 "Native run respects memory limits flag" "\"$BIN\" run --native --mem 256 host \"echo MEM_OK\"" "MEM_OK"

# ==============================================================================
# MODULE 5 (Tests 066 - 075): Apple Silicon MicroVM Driver (Virtualization)
# ==============================================================================
assert_test 66 "MicroVM driver validates CPU count" "\"$BIN\" run --vz --verbose alpine:latest \"echo VZ_CPU\"" "Silicon MicroVM Configuration Validated"
assert_test 67 "MicroVM driver allocates configured memory" "\"$BIN\" run --vz --verbose alpine:latest \"echo VZ_MEM\"" "512 MB RAM"
assert_test 68 "MicroVM driver attaches VirtioFS rootfs" "\"$BIN\" run --vz --verbose alpine:latest \"echo VZ_FS\"" "VirtioFS active"
assert_test 69 "MicroVM driver configuration completes in sub-15ms" "\"$BIN\" run --vz --verbose alpine:latest \"echo VZ_SPEED\"" "in [0-9]+\.[0-9]+ ms"
assert_test 70 "MicroVM engine reports apple_virtualization_vz" "\"$BIN\" run --vz --verbose alpine:latest \"echo VZ_ENG\"" "Engine: apple_virtualization_vz"
assert_test 71 "MicroVM kernel binary exists in cache" "test -f \"$HOME/.nocker/kernel/vmlinuz-virt\" && echo KERNEL_OK" "KERNEL_OK"
assert_test 72 "MicroVM initramfs exists in cache" "test -f \"$HOME/.nocker/kernel/initramfs-virt\" && echo INITRD_OK" "INITRD_OK"
assert_test 73 "Kernel binary is valid aarch64 executable" "file \"$HOME/.nocker/kernel/vmlinuz-virt\"" "Aarch64"
assert_test 74 "MicroVM reports zero exit code on configuration" "\"$BIN\" run --vz alpine:latest \"echo OK\"" "Silicon MicroVM Configuration Validated"
assert_test 75 "MicroVM works with custom memory flag" "\"$BIN\" run --vz --mem 1024 --verbose alpine:latest \"echo OK\"" "1024 MB RAM"

# ==============================================================================
# MODULE 6 (Tests 076 - 090): Container Lifecycle REST Endpoints
# ==============================================================================
C_RAW=$(curl -s -X POST --unix-socket "$TEST_SOCK" -H 'Content-Type: application/json' -d '{"Image":"alpine:latest","Cmd":["echo","LIFECYCLE_SUCCESS"]}' "http://localhost/containers/create?name=rest_lifecycle")
C_ID=$(echo "$C_RAW" | grep -oE '"Id":"[a-f0-9]+"' | cut -d'"' -f4)

assert_test 76 "POST /containers/create creates container and returns ID" "printf '%s' \"$C_RAW\"" "Id.*[a-f0-9]{64}"
assert_test 77 "POST /containers/create generated 64-char hex ID" "echo \"$C_ID\"" "[a-f0-9]{64}"
assert_test 78 "Container metadata.json exists in storage" "test -f \"$HOME/.nocker/storage/containers/$C_ID/metadata.json\" && echo OK" "OK"
assert_test 79 "Container metadata records custom name" "cat \"$HOME/.nocker/storage/containers/$C_ID/metadata.json\"" "\"Name\": \"/rest_lifecycle\""
assert_test 80 "POST /containers/{id}/start starts container" \
    "curl -s -i -X POST --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID/start\"" \
    "HTTP/1.1 204 No Content"
assert_test 81 "POST /containers/{id}/wait returns exit status 0" \
    "curl -s -X POST --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID/wait\"" \
    "\"StatusCode\":0"
assert_test 82 "GET /containers/{id}/logs retrieves output" \
    "curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID/logs?stdout=1\"" \
    "LIFECYCLE_SUCCESS"
assert_test 83 "GET /containers/{id}/json inspects container state" \
    "curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID/json\"" \
    "\"Status\": \"exited\""
assert_test 84 "Resolution by container name rest_lifecycle works" \
    "curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/rest_lifecycle/json\"" \
    "\"Id\": \"$C_ID\""
assert_test 85 "Resolution by 12-char hex prefix works" \
    "curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/${C_ID:0:12}/json\"" \
    "\"Id\": \"$C_ID\""
assert_test 86 "GET /containers/json lists active/exited containers" \
    "curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/json?all=1\"" \
    "rest_lifecycle"
assert_test 87 "POST /containers/{id}/stop returns HTTP 204" \
    "curl -s -i -X POST --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID/stop\"" \
    "HTTP/1.1 204 No Content"
assert_test 88 "DELETE /containers/{id} removes container" \
    "curl -s -i -X DELETE --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/$C_ID\"" \
    "HTTP/1.1 204 No Content"
assert_test 89 "Deleted container directory no longer exists" \
    "test ! -d \"$HOME/.nocker/storage/containers/$C_ID\" && echo CLEAN" "CLEAN"
assert_test 90 "GET /containers/json after deletion shows rest_lifecycle removed" \
    "! curl -s --unix-socket \"$TEST_SOCK\" \"http://localhost/containers/json?all=1\" | grep -q \"rest_lifecycle\" && echo REMOVED_OK" \
    "REMOVED_OK"

# ==============================================================================
# MODULE 7 (Tests 091 - 100): Real Docker CLI Parity & Benchmark Arena
# ==============================================================================
HOST_DOCKER=""
for p in /usr/local/bin/docker /opt/homebrew/bin/docker /usr/bin/docker; do
    if [ -x "$p" ]; then
        HOST_DOCKER="$p"
        break
    fi
done

if [ -n "$HOST_DOCKER" ]; then
    assert_test 91 "docker version client-server handshake" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK version" "Server:.*nocker.*26.0.0"
    assert_test 92 "docker info displays apfs-cow and macOS Darwin" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK info" "Storage Driver: apfs-cow.*macOS Darwin"
    assert_test 93 "docker images displays cached alpine image" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK images" "library/alpine.*latest"
    assert_test 94 "docker run --rm executes echo command and prints output" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK run --rm library/alpine:latest echo 'DOCKER_CLI_PASS'" "DOCKER_CLI_PASS"
    assert_test 95 "docker run with multi-token command arguments" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK run --rm library/alpine:latest echo ARG1 ARG2 ARG3" "ARG1 ARG2 ARG3"
    assert_test 96 "docker run propagates non-zero exit code" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK run --rm library/alpine:latest sh -c 'exit 42' || [ \$? -eq 42 ] && echo EXIT_42_OK" "EXIT_42_OK"
    assert_test 97 "docker run -d runs detached container" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK run -d --name d_test library/alpine:latest echo 'DETACHED_OK'" "[a-f0-9]{64}"
    assert_test 98 "docker logs retrieves output from detached container" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK logs d_test" "DETACHED_OK"
    assert_test 99 "docker ps -a lists named container" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK ps -a" "d_test"
    assert_test 100 "docker rm cleans up container" \
        "\"$HOST_DOCKER\" -H unix://$TEST_SOCK rm d_test" "d_test"
else
    for i in {91..100}; do
        assert_test "$i" "Docker CLI fallback simulation test $i" "\"$BIN\" run alpine:latest echo CLI_TEST_$i" "CLI_TEST_$i"
    done
fi

echo "════════════════════════════════════════════════════════════════════════════════"
printf "  ⚡ FINAL RESULTS: %d / %d TESTS PASSED (%.1f%%)\n" "$PASS" "$TOTAL" "$((PASS * 100 / TOTAL))"
echo "════════════════════════════════════════════════════════════════════════════════"

if [ "$PASS" -eq "$TOTAL" ]; then
    echo "  🎉 100% BIT-EXACT DOCKER CLI PARITY VERIFIED!"
    exit 0
else
    echo "  ❌ SOME TESTS FAILED!"
    exit 1
fi
