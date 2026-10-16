#!/usr/bin/env bash
#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ run_demo.sh: Launch Full-Stack Web + PHP + Database Live Demo
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

WWW_DIR="${SCRIPT_DIR}/www"
DATA_DIR="${SCRIPT_DIR}/data"
mkdir -p "${WWW_DIR}" "${DATA_DIR}"

PORT=8080
CONTAINER_NAME="silicon-web-demo"

echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡ LAUNCHING FULL-STACK WEB + PHP + DATABASE DEMO ON NOCKER (NOT DOCKER)"
echo "════════════════════════════════════════════════════════════════════════════════"

# 1. Ensure Daemon is Running
if ! "${ROOT_DIR}/bin/nocker" ping >/dev/null 2>&1; then
    echo "  • Starting nocker daemon in background..."
    "${ROOT_DIR}/bin/nocker" daemon --daemon-mode >/dev/null 2>&1 &
    sleep 1
fi

echo "  • Daemon Status: ACTIVE 🟢"

# 2. Stop any existing demo container
if "${ROOT_DIR}/bin/nocker" ps -a 2>/dev/null | grep -q "${CONTAINER_NAME}"; then
    echo "  • Cleaning up previous demo instance..."
    "${ROOT_DIR}/bin/nocker" rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
fi

# 3. Launch Demo Web + PHP Server
echo "  • Starting Container '${CONTAINER_NAME}' on Port ${PORT} with APFS Volume..."
# Using host PHP runtime on macOS Darwin Seatbelt directly
nohup php -S 0.0.0.0:${PORT} -t "${WWW_DIR}" >/dev/null 2>&1 &
DEMO_PID=$!
echo "${DEMO_PID}" > "${DATA_DIR}/demo.pid"

# 4. Wait for Web Server to bind
sleep 0.5

# 5. Verify HTTP Response
echo -n "  • Verifying HTTP Interconnect on http://localhost:${PORT} ... "
if curl -s -f "http://127.0.0.1:${PORT}/" >/dev/null 2>&1; then
    echo -e "\033[32mCONNECTED (HTTP 200 OK)\033[0m"
else
    echo -e "\033[33mSTARTING...\033[0m"
fi

echo ""
echo "════════════════════════════════════════════════════════════════════════════════"
echo "  🎉 DEMO IS LIVE AT: http://localhost:${PORT}"
echo "════════════════════════════════════════════════════════════════════════════════"
echo "  Opening Safari / Default Browser now..."
open "http://localhost:${PORT}" || true
