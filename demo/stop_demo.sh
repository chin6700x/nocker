#!/usr/bin/env bash
#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ stop_demo.sh: Stop and clean up the Live Demo instance
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DATA_DIR="${SCRIPT_DIR}/data"

echo "  • Stopping Live Demo server..."

if [ -f "${DATA_DIR}/demo.pid" ]; then
    PID=$(cat "${DATA_DIR}/demo.pid")
    if kill -0 "${PID}" 2>/dev/null; then
        kill "${PID}" 2>/dev/null || true
        echo "  ✓ Stopped Demo Process (PID: ${PID})"
    fi
    rm -f "${DATA_DIR}/demo.pid"
fi

# Kill any stray php listening on 8080
lsof -ti :8080 | xargs kill -9 2>/dev/null || true

echo "  ✓ Demo cleanup complete."
