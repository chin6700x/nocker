/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Unified Twin-Engine Dispatcher Header
 */

#ifndef ENGINE_ROUTER_H
#define ENGINE_ROUTER_H

#include "nocker.h"
#include "oci_spec.h"
#include "vz_bridge.h"
#include "native_seatbelt.h"

typedef struct {
    char image_name[256];
    char command[2048];
    char container_id[128];
    int timeout_ms;
    int max_mem_mb;
    bool force_native;
    bool force_vz;
    char bind_host[256];
    char bind_cnt[256];
} container_run_spec_t;

typedef struct {
    int exit_code;
    double duration_ms;
    bool timed_out;
    char engine_used[64];   /* "vz_microvm" or "darwin_seatbelt" */
    char container_rootfs[1024];
    char stdout_buf[65536];
    char stderr_buf[16384];
    char error_msg[512];
} container_run_result_t;

int engine_dispatch_run(const engine_config_t *cfg,
                        const container_run_spec_t *spec,
                        container_run_result_t *res);

#endif /* ENGINE_ROUTER_H */
