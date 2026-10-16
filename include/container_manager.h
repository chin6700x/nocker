/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Container Lifecycle Manager Interface
 */

#ifndef CONTAINER_MANAGER_H
#define CONTAINER_MANAGER_H

#include "nocker.h"
#include "engine_router.h"
#include <time.h>

typedef struct {
    char id[65];             /* 64-hex container identifier */
    char name[64];           /* container name */
    char image[256];         /* image name e.g. alpine:latest */
    char command[1024];      /* command to execute */
    char rootfs_path[1024];  /* APFS CoW cloned path */
    bool running;
    int exit_code;
    time_t created_at;
    time_t finished_at;
    bool force_vz;
    bool force_native;
} container_info_t;

/* Container Management Operations */
int cm_init(const char *storage_root);
int cm_resolve_id(const char *storage_root,
                  const char *input,
                  char *out_id,
                  size_t out_len);

int cm_create_container(const char *storage_root,
                        const char *image,
                        const char *command,
                        const char *name_hint,
                        char *out_id,
                        size_t id_len);

int cm_start_container(const char *storage_root,
                       const char *id,
                       container_run_result_t *res);

int cm_wait_container(const char *storage_root,
                      const char *id,
                      int *exit_code);

int cm_get_logs(const char *storage_root,
                const char *id,
                bool stdout_flag,
                bool stderr_flag,
                bool raw_tty,
                char **out_buf,
                size_t *out_len);

int cm_inspect_json(const char *storage_root,
                    const char *id,
                    char *out_buf,
                    size_t max_len);

int cm_list_containers_json(const char *storage_root,
                            bool all,
                            char *out_buf,
                            size_t max_len);

int cm_remove_container(const char *storage_root,
                        const char *id,
                        bool force);

int cm_stop_container(const char *storage_root,
                      const char *id);

int cm_bind_mount(const char *storage_root,
                  const char *id,
                  const char *host_path,
                  const char *container_path);

int cm_copy_to_container(const char *storage_root,
                         const char *id,
                         const char *src_host,
                         const char *dest_container);

int cm_copy_from_container(const char *storage_root,
                           const char *id,
                           const char *src_container,
                           const char *dest_host);

int cm_extract_tar_to_container(const char *storage_root,
                                const char *id,
                                const char *target_dir,
                                const char *tar_data,
                                size_t tar_len);

#endif /* CONTAINER_MANAGER_H */
