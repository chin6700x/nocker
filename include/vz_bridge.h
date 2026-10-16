/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Apple Virtualization.framework Bridge Header
 */

#ifndef VZ_BRIDGE_H
#define VZ_BRIDGE_H

#include "nocker.h"

typedef struct {
    int cpus;
    uint64_t memory_mb;
    char kernel_path[1024];
    char initrd_path[1024];
    char cmdline[1024];
    char virtiofs_tag[128];
    char virtiofs_path[1024];
    bool virtiofs_readonly;
    int timeout_ms;
} vz_config_t;

typedef struct {
    bool is_supported;
    bool is_running;
    int exit_code;
    double boot_time_ms;
    char error_msg[512];
    char stdout_buf[65536];
    char stderr_buf[16384];
} vz_result_t;

/* Probe Apple Silicon Virtualization */
int vz_probe_support(void);

/* Boot and run microVM */
int vz_run_microvm(const vz_config_t *cfg, vz_result_t *res);

#endif /* VZ_BRIDGE_H */
