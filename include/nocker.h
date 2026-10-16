/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Core Engine & Interfaces
 */

#ifndef NOCKER_H
#define NOCKER_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define ENGINE_NAME          "nocker"
#define ENGINE_VERSION       "1.0.0"
#define DOCKER_API_VERSION   "1.45"
#define DOCKER_ENGINE_VER    "26.0.0"
#define GIT_COMMIT           "chin6700x"

#define DEFAULT_USER_SOCK_DIR   "/.nocker/run"
#define DEFAULT_SOCK_FILE       "nocker.sock"
#define DOCKER_COMPAT_SOCK      "docker.sock"
#define GLOBAL_DOCKER_SOCK      "/var/run/docker.sock"

typedef struct {
    char socket_path[1024];
    bool daemon_mode;
    bool verbose;
    bool docker_compat; /* create symlink to ~/.nocker/run/docker.sock and /var/run/docker.sock */
    int port; /* optional TCP port (default: 0 = disabled) */
    char storage_root[1024];
} engine_config_t;

/* Global Engine API */
int engine_init_defaults(engine_config_t *cfg);
int engine_run_daemon(const engine_config_t *cfg);
const char *engine_get_scratch_base(void);
double engine_get_time_ms(void);

#endif /* NOCKER_H */
