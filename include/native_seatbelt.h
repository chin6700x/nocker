/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Native Darwin Seatbelt & Watchdog Runner Header
 */

#ifndef NATIVE_SEATBELT_H
#define NATIVE_SEATBELT_H

#include "nocker.h"

typedef struct {
    char command[2048];
    char workdir[1024];
    int timeout_ms;
    int max_mem_mb;
    bool strict_sandbox;
    char env_keys[16][128];
    char env_vals[16][256];
    int env_count;
} seatbelt_spec_t;

typedef struct {
    int exit_code;
    double exec_time_ms;
    bool timed_out;
    int term_signal;
    char stdout_buf[65536];
    char stderr_buf[16384];
    char error_msg[512];
} seatbelt_result_t;

int native_seatbelt_run(const seatbelt_spec_t *spec, seatbelt_result_t *res);

#endif /* NATIVE_SEATBELT_H */
