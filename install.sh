#!/bin/bash
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ nocker: Not Docker · Container Engine Installer
# One-line installer: curl -fsSL https://raw.githubusercontent.com/chin6700x/nocker/main/install.sh | bash

set -e

BOLD="\033[1m"
GREEN="\033[0;32m"
YELLOW="\033[0;33m"
CYAN="\033[0;36m"
RED="\033[0;31m"
RESET="\033[0m"

echo -e "${CYAN}${BOLD}"
echo "════════════════════════════════════════════════════════════════════════════════"
echo "  ⚡  INSTALLING NOCKER (NOT DOCKER) · BARE-METAL APPLE SILICON ENGINE"
echo "════════════════════════════════════════════════════════════════════════════════"
echo -e "${RESET}"

# 1. Architecture & Platform Verification
OS_NAME=$(uname -s)
ARCH_NAME=$(uname -m)

if [ "$OS_NAME" != "Darwin" ]; then
    echo -e "${RED}❌ Error: Nocker is exclusively engineered for macOS Darwin.${RESET}"
    exit 1
fi

if [ "$ARCH_NAME" != "arm64" ]; then
    echo -e "${YELLOW}⚠️  Warning: Non-ARM64 architecture detected ($ARCH_NAME). Apple Silicon is strongly recommended.${RESET}"
fi

# 2. Dependency Check
if ! command -v clang >/dev/null 2>&1; then
    echo -e "${YELLOW}⚡ Installing Xcode Command Line Tools...${RESET}"
    xcode-select --install || true
fi

# 3. Installation Paths
INSTALL_DIR="${HOME}/.local/bin"
NOCKER_HOME="${HOME}/.nocker"
mkdir -p "${INSTALL_DIR}"
mkdir -p "${NOCKER_HOME}/run"
mkdir -p "${NOCKER_HOME}/storage/images"
mkdir -p "${NOCKER_HOME}/storage/containers"
mkdir -p "${NOCKER_HOME}/tools"

# 4. Compilation or In-Place Install
if [ -f "Makefile" ] && [ -d "src" ]; then
    echo -e "${CYAN}⚡ Building nocker from local source...${RESET}"
    make all
    make install
else
    echo -e "${CYAN}⚡ Fetching latest nocker release...${RESET}"
    TMP_DIR=$(mktemp -d)
    git clone --depth 1 https://github.com/chin6700x/nocker.git "${TMP_DIR}/nocker"
    cd "${TMP_DIR}/nocker"
    make all
    make install
    rm -rf "${TMP_DIR}"
fi

# 5. Shell PATH Warning
if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
    echo -e "${YELLOW}ℹ️  Add ~/.local/bin to your PATH if not already present:${RESET}"
    echo -e "   export PATH=\"\$HOME/.local/bin:\$PATH\""
fi

echo ""
echo -e "${GREEN}${BOLD}✓ Nocker installed successfully!${RESET}"
echo ""
"${INSTALL_DIR}/nocker" doctor || true

echo ""
echo -e "${CYAN}${BOLD}🚀 Quickstart Guide:${RESET}"
echo -e "  1. Start daemon       : ${BOLD}nocker daemon &${RESET}"
echo -e "  2. Use with Docker CLI : ${BOLD}export DOCKER_HOST=\"unix://\$HOME/.nocker/run/docker.sock\"${RESET}"
echo -e "  3. Import from Docker : ${BOLD}nocker import-docker --all${RESET}"
echo -e "  4. Multi-language test : ${BOLD}make test-languages${RESET}"
echo ""
echo -e "${CYAN}Enjoy bare-metal container velocity on Apple Silicon! ⚡${RESET}"
