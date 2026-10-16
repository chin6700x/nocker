#
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
#
# ⚡ nocker: Apple Silicon Bare-Metal Container Engine Unified Makefile
#

CC = clang
CFLAGS = -std=c11 -O3 -Wall -Wextra -MMD -MP -Iinclude -Isrc/utils -Isrc/storage -Isrc/engine -Isrc/vz
OBJCFLAGS = -O3 -Wall -Wextra -MMD -MP -Iinclude -Isrc/vz
LDFLAGS = -lcurl -framework Foundation -framework Virtualization

BIN_DIR = bin
BUILD_DIR = build
TARGET = $(BIN_DIR)/nocker

C_SRCS = src/main.c \
         src/utils/http_parser.c \
         src/socket/dockersock.c \
         src/socket/docker_routes.c \
         src/storage/registry_client.c \
         src/storage/apfs_cow.c \
         src/engine/native_seatbelt.c \
         src/engine/engine_router.c \
         src/engine/container_manager.c

OBJC_SRCS = src/vz/vz_vm.m

OBJS = $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SRCS)) \
       $(patsubst src/%.m,$(BUILD_DIR)/%.o,$(OBJC_SRCS))

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	@echo "  ⚡ [LD] $@"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo "  ⚡ [CODESIGN] $@ (Apple Virtualization Entitlements)"
	@codesign -s - --entitlements entitlements.plist -f $@ 2>/dev/null || true

$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "  ⚡ [CC] $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.m | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	@echo "  ⚡ [OBJC] $<"
	@$(CC) $(OBJCFLAGS) -c $< -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "  ✓ Cleaned build artifacts"

DEPS = $(OBJS:.o=.d)
-include $(DEPS)

test: all
	@echo "════════════════════════════════════════════════════════════════════════════════"
	@echo "  ⚡ RUNNING NOCKER TEST SUITE"
	@echo "════════════════════════════════════════════════════════════════════════════════"
	@bash test/test_nockersock.sh
	@bash test/test_parity.sh

test-languages: all
	@bash test/test_languages.sh

install: all
	@mkdir -p $(HOME)/.local/bin
	@mkdir -p $(HOME)/.nocker/tools
	@cp -f $(TARGET) $(HOME)/.local/bin/nocker
	@cp -f tools/import_docker.sh $(HOME)/.nocker/tools/import_docker.sh
	@chmod +x $(HOME)/.nocker/tools/import_docker.sh
	@echo "  ✓ Installed nocker to $(HOME)/.local/bin/nocker"

uninstall:
	@rm -f $(HOME)/.local/bin/nocker
	@rm -rf $(HOME)/.nocker/tools
	@echo "  ✓ Uninstalled nocker"

.PHONY: all clean test test-languages install uninstall
