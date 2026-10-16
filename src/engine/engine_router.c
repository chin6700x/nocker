/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Unified Twin-Engine Dispatcher Implementation
 */

#include "engine_router.h"

int engine_dispatch_run(const engine_config_t *cfg,
                        const container_run_spec_t *spec,
                        container_run_result_t *res) {
    if (!cfg || !spec || !res) return -1;
    memset(res, 0, sizeof(container_run_result_t));

    char cnt_id[128];
    if (spec->container_id[0]) {
        snprintf(cnt_id, sizeof(cnt_id), "%s", spec->container_id);
    } else {
        snprintf(cnt_id, sizeof(cnt_id), "cnt_%ld_%d", (long)time(NULL), getpid());
    }

    /* Route 1: Bare-Metal Native Seatbelt Engine (Speed < 2ms) */
    if (spec->force_native ||
        strcmp(spec->image_name, "native") == 0 ||
        strcmp(spec->image_name, "host") == 0) {
        
        seatbelt_spec_t s_spec;
        memset(&s_spec, 0, sizeof(s_spec));
        snprintf(s_spec.command, sizeof(s_spec.command), "%s", spec->command);
        s_spec.timeout_ms = spec->timeout_ms > 0 ? spec->timeout_ms : 5000;
        s_spec.max_mem_mb = spec->max_mem_mb > 0 ? spec->max_mem_mb : 512;
        s_spec.strict_sandbox = true;

        seatbelt_result_t s_res;
        int rc = native_seatbelt_run(&s_spec, &s_res);

        res->exit_code = s_res.exit_code;
        res->duration_ms = s_res.exec_time_ms;
        res->timed_out = s_res.timed_out;
        snprintf(res->engine_used, sizeof(res->engine_used), "darwin_seatbelt_native");
        snprintf(res->stdout_buf, sizeof(res->stdout_buf), "%s", s_res.stdout_buf);
        snprintf(res->stderr_buf, sizeof(res->stderr_buf), "%s", s_res.stderr_buf);
        snprintf(res->error_msg, sizeof(res->error_msg), "%s", s_res.error_msg);
        return rc;
    }

    /* Route 2: OCI Container with APFS CoW Layer Isolation */
    oci_image_metadata_t meta;
    if (oci_get_image_metadata(cfg->storage_root, spec->image_name, &meta) != 0) {
        /* Auto-pull image if missing */
        printf("⚡ [engine] Image '%s' not found locally. Auto-pulling...\n", spec->image_name);
        oci_image_ref_t ref;
        oci_parse_image_ref(spec->image_name, &ref);
        if (oci_pull_image(&ref, cfg->storage_root, &meta) != 0) {
            snprintf(res->error_msg, sizeof(res->error_msg), "Failed to auto-pull image %s", spec->image_name);
            res->exit_code = 1;
            return -1;
        }
    }

    /* Instant APFS CoW Snapshot */
    char rootfs_path[1024];
    if (apfs_create_container_scratch(cfg->storage_root, spec->image_name, cnt_id,
                                     rootfs_path, sizeof(rootfs_path)) != 0) {
        snprintf(res->error_msg, sizeof(res->error_msg), "Failed to clone APFS rootfs for %s", cnt_id);
        res->exit_code = 1;
        return -1;
    }
    snprintf(res->container_rootfs, sizeof(res->container_rootfs), "%s", rootfs_path);

    /* Bind Mount if specified */
    if (spec->bind_host[0] && spec->bind_cnt[0]) {
        const char *rel = spec->bind_cnt;
        while (*rel == '/') rel++;
        char target[1024];
        snprintf(target, sizeof(target), "%s/%s", rootfs_path, rel);
        char dcmd[1100];
        snprintf(dcmd, sizeof(dcmd), "mkdir -p \"$(dirname '%s')\"", target);
        system(dcmd);
        char lcmd[2200];
        snprintf(lcmd, sizeof(lcmd), "ln -sfn \"%s\" \"%s\"", spec->bind_host, target);
        system(lcmd);
    }

    /* If --vz specified: execute via Apple Virtualization Driver */
    if (spec->force_vz) {
        vz_config_t vz_cfg;
        memset(&vz_cfg, 0, sizeof(vz_cfg));
        vz_cfg.cpus = 2;
        vz_cfg.memory_mb = spec->max_mem_mb > 0 ? (uint64_t)spec->max_mem_mb : 512;
        /* Locate Linux ARM64 microvm kernel */
        const char *home = getenv("HOME");
        if (!home) home = "/tmp";
        char k_path[1024];
        char initrd_path[1024];
        snprintf(k_path, sizeof(k_path), "%s/.nocker/kernel/vmlinuz-virt", home);
        snprintf(initrd_path, sizeof(initrd_path), "%s/.nocker/kernel/initramfs-virt", home);

        if (access(k_path, R_OK) == 0) {
            snprintf(vz_cfg.kernel_path, sizeof(vz_cfg.kernel_path), "%s", k_path);
        }
        if (access(initrd_path, R_OK) == 0) {
            snprintf(vz_cfg.initrd_path, sizeof(vz_cfg.initrd_path), "%s", initrd_path);
        }
        snprintf(vz_cfg.virtiofs_tag, sizeof(vz_cfg.virtiofs_tag), "rootfs");
        snprintf(vz_cfg.virtiofs_path, sizeof(vz_cfg.virtiofs_path), "%s", rootfs_path);
        vz_cfg.virtiofs_readonly = false;

        vz_result_t vz_res;
        int rc = vz_run_microvm(&vz_cfg, &vz_res);

        res->exit_code = vz_res.exit_code;
        res->duration_ms = vz_res.boot_time_ms;
        snprintf(res->engine_used, sizeof(res->engine_used), "apple_virtualization_vz");
        snprintf(res->stdout_buf, sizeof(res->stdout_buf), "%s", vz_res.stdout_buf);
        snprintf(res->stderr_buf, sizeof(res->stderr_buf), "%s", vz_res.stderr_buf);
        snprintf(res->error_msg, sizeof(res->error_msg), "%s", vz_res.error_msg);
        return rc;
    }

    /* Fast Container Sandbox Execution inside the APFS rootfs */
    seatbelt_spec_t s_spec;
    memset(&s_spec, 0, sizeof(s_spec));
    snprintf(s_spec.command, sizeof(s_spec.command), "%s", spec->command);
    snprintf(s_spec.workdir, sizeof(s_spec.workdir), "%s", rootfs_path);
    s_spec.timeout_ms = spec->timeout_ms > 0 ? spec->timeout_ms : 5000;
    s_spec.max_mem_mb = spec->max_mem_mb > 0 ? spec->max_mem_mb : 512;
    s_spec.strict_sandbox = false; /* container has its own rootfs directory */

    seatbelt_result_t s_res;
    int rc = native_seatbelt_run(&s_spec, &s_res);

    res->exit_code = s_res.exit_code;
    res->duration_ms = s_res.exec_time_ms;
    res->timed_out = s_res.timed_out;
    snprintf(res->engine_used, sizeof(res->engine_used), "apfs_cow_container");
    snprintf(res->stdout_buf, sizeof(res->stdout_buf), "%s", s_res.stdout_buf);
    snprintf(res->stderr_buf, sizeof(res->stderr_buf), "%s", s_res.stderr_buf);
    snprintf(res->error_msg, sizeof(res->error_msg), "%s", s_res.error_msg);
    return rc;
}
