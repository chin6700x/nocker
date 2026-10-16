#!/bin/bash
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ nocker: Not Docker · Zero-Download Docker Desktop Importer
# Streams container rootfs directly from Docker Desktop into APFS storage at Silicon speed.

set -e

STORAGE_ROOT="${HOME}/.nocker/storage"
IMAGES_DIR="${STORAGE_ROOT}/images"
JSON_FILE="${STORAGE_ROOT}/images.json"

mkdir -p "${IMAGES_DIR}"

if ! docker info >/dev/null 2>&1; then
    echo "❌ Error: Docker Desktop is not running or socket not accessible."
    exit 1
fi

import_single_image() {
    local RAW_IMG="$1"
    
    # Parse tag
    local REPO=""
    local TAG=""
    if [[ "$RAW_IMG" == *:* ]]; then
        REPO="${RAW_IMG%:*}"
        TAG="${RAW_IMG##*:}"
    else
        REPO="$RAW_IMG"
        TAG="latest"
    fi

    # Docker official library prefixing
    local FULL_REPO="$REPO"
    if [[ "$REPO" != *"/"* ]]; then
        FULL_REPO="library/${REPO}"
    fi

    local SAFE_REPO="${FULL_REPO//\//_}"
    SAFE_REPO="${SAFE_REPO//:/_}"
    
    local TARGET_DIR="${IMAGES_DIR}/${SAFE_REPO}_${TAG}"
    local ROOTFS_DIR="${TARGET_DIR}/rootfs"

    echo "⚡ [nocker-import] Importing '${RAW_IMG}' (-> ${SAFE_REPO}_${TAG})..."

    # Inspect image size and id
    local IMG_ID=$(docker inspect --format='{{.Id}}' "$RAW_IMG" 2>/dev/null || echo "")
    if [ -z "$IMG_ID" ]; then
        echo "❌ Image '$RAW_IMG' not found in Docker Desktop."
        return 1
    fi
    local IMG_SIZE=$(docker inspect --format='{{.Size}}' "$RAW_IMG" 2>/dev/null || echo "0")
    local IMG_CREATED=$(docker inspect --format='{{.Created}}' "$RAW_IMG" 2>/dev/null || echo "")

    local T0=$(python3 -c 'import time; print(time.time())')

    # Create temporary container
    local CID=$(docker create "$RAW_IMG")
    if [ -z "$CID" ]; then
        echo "❌ Failed to create temporary container for '$RAW_IMG'."
        return 1
    fi

    rm -rf "${ROOTFS_DIR}"
    mkdir -p "${ROOTFS_DIR}"

    # Stream rootfs directly via memory pipe
    docker export "$CID" | tar -xf - -C "${ROOTFS_DIR}"
    docker rm "$CID" >/dev/null

    # Calculate real extracted size
    local DISK_KB=$(du -sk "${ROOTFS_DIR}" | awk '{print $1}')
    local REAL_BYTES=$((DISK_KB * 1024))
    if [ "$IMG_SIZE" -lt 1048576 ] 2>/dev/null; then
        IMG_SIZE=$REAL_BYTES
    fi

    local T1=$(python3 -c 'import time; print(time.time())')
    local ELAPSED=$(python3 -c "print(f'{$T1 - $T0:.2f}')")
    local SIZE_MB=$(python3 -c "print(f'{$IMG_SIZE / (1024*1024):.1f}')")

    # Update images.json via python helper
    python3 - <<EOF
import json, os, time

json_path = os.path.expanduser("${JSON_FILE}")
entries = []
if os.path.exists(json_path):
    try:
        with open(json_path, "r") as f:
            entries = json.load(f)
    except Exception:
        entries = []

# Remove existing duplicate
entries = [e for e in entries if "${RAW_IMG}" not in e.get("RepoTags", []) and "${FULL_REPO}:${TAG}" not in e.get("RepoTags", [])]

new_entry = {
    "Id": "${IMG_ID}",
    "RepoTags": ["${FULL_REPO}:${TAG}", "${RAW_IMG}"],
    "Created": int(time.time()),
    "Size": int("${IMG_SIZE}"),
    "VirtualSize": int("${IMG_SIZE}"),
    "RootFS": "${ROOTFS_DIR}"
}
entries.append(new_entry)

with open(json_path, "w") as f:
    json.dump(entries, f, indent=2)
EOF

    echo "✅ [nocker-import] Successfully imported '${RAW_IMG}' (${SIZE_MB} MB) in ${ELAPSED}s -> ${ROOTFS_DIR}"
}

if [ "$1" == "--all" ]; then
    echo "⚡ [nocker-import] Scanning all Docker Desktop images..."
    # Get all repository:tag
    IMAGES=$(docker images --format "{{.Repository}}:{{.Tag}}" | grep -v "<none>")
    for img in $IMAGES; do
        import_single_image "$img" || true
    done
elif [ -n "$1" ]; then
    import_single_image "$1"
else
    echo "Usage:"
    echo "  $0 <image_name:tag>   # Import a specific image from Docker Desktop"
    echo "  $0 --all              # Import all local images from Docker Desktop"
    exit 1
fi
