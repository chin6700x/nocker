/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Container Lifecycle Manager Implementation
 */

#include "container_manager.h"
#include "oci_spec.h"
#include <dirent.h>
#include <fcntl.h>

static void generate_container_id(char *out, size_t len) {
    if (len < 65) return;
    static const char hex[] = "0123456789abcdef";
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((unsigned int)(tv.tv_usec ^ getpid()));
    for (int i = 0; i < 64; i++) {
        out[i] = hex[rand() % 16];
    }
    out[64] = '\0';
}

static void json_escape_string(const char *in, char *out, size_t max) {
    if (!in || !out || max == 0) return;
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 2 < max; i++) {
        if (in[i] == '"') {
            out[o++] = '\\';
            out[o++] = '"';
        } else if (in[i] == '\\') {
            out[o++] = '\\';
            out[o++] = '\\';
        } else if (in[i] == '\n') {
            out[o++] = '\\';
            out[o++] = 'n';
        } else if (in[i] == '\r') {
            out[o++] = '\\';
            out[o++] = 'r';
        } else if (in[i] == '\t') {
            out[o++] = '\\';
            out[o++] = 't';
        } else {
            out[o++] = in[i];
        }
    }
    out[o] = '\0';
}

static void ensure_dir(const char *path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", path);
    (void)system(cmd);
}

int cm_init(const char *storage_root) {
    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers", storage_root);
    ensure_dir(c_dir);
    return 0;
}

int cm_resolve_id(const char *storage_root,
                  const char *input,
                  char *out_id,
                  size_t out_len) {
    if (!storage_root || !input || !out_id || out_len < 65) return -1;
    out_id[0] = '\0';

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers", storage_root);

    /* 1. Direct match on directory */
    char direct[1024];
    snprintf(direct, sizeof(direct), "%s/%s/metadata.json", c_dir, input);
    if (access(direct, R_OK) == 0) {
        snprintf(out_id, out_len, "%s", input);
        return 0;
    }

    /* 2. Search container directory for prefix or name match */
    DIR *d = opendir(c_dir);
    if (!d) return -1;

    size_t in_len = strlen(input);
    const char *clean_in = (input[0] == '/') ? input + 1 : input;
    struct dirent *de;

    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;

        /* ID prefix match */
        if (strncmp(de->d_name, input, in_len) == 0) {
            snprintf(out_id, out_len, "%s", de->d_name);
            closedir(d);
            return 0;
        }

        /* Name match in metadata */
        char meta_path[1024];
        snprintf(meta_path, sizeof(meta_path), "%s/%s/metadata.json", c_dir, de->d_name);
        FILE *fp = fopen(meta_path, "r");
        if (fp) {
            char buf[2048] = {0};
            fread(buf, 1, sizeof(buf) - 1, fp);
            fclose(fp);

            char *pn = strstr(buf, "\"Name\": \"");
            if (pn) {
                char name[64] = {0};
                sscanf(pn + 9, "%63[^\"]", name);
                const char *cn = (name[0] == '/') ? name + 1 : name;
                if (strcmp(cn, clean_in) == 0 || strcmp(name, input) == 0) {
                    snprintf(out_id, out_len, "%s", de->d_name);
                    closedir(d);
                    return 0;
                }
            }
        }
    }
    closedir(d);
    return -1;
}

int cm_create_container(const char *storage_root,
                        const char *image,
                        const char *command,
                        const char *name_hint,
                        char *out_id,
                        size_t id_len) {
    if (!storage_root || !image || !out_id || id_len < 65) return -1;

    cm_init(storage_root);
    generate_container_id(out_id, id_len);

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers/%s", storage_root, out_id);
    ensure_dir(c_dir);

    char name[64];
    if (name_hint && name_hint[0]) {
        snprintf(name, sizeof(name), "%s", name_hint);
    } else {
        snprintf(name, sizeof(name), "silicon_cnt_%.12s", out_id);
    }

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/rootfs", c_dir);

    /* Instant APFS CoW Layer Snapshot */
    if (strcmp(image, "native") != 0 && strcmp(image, "host") != 0) {
        apfs_create_container_scratch(storage_root, image, out_id, rootfs, sizeof(rootfs));
    }

    /* Save Metadata */
    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.json", c_dir);
    FILE *fp = fopen(meta_path, "w");
    if (fp) {
        time_t now = time(NULL);
        char esc_cmd[2048] = {0};
        json_escape_string(command ? command : "", esc_cmd, sizeof(esc_cmd));
        fprintf(fp,
            "{\n"
            "  \"Id\": \"%s\",\n"
            "  \"Name\": \"/%s\",\n"
            "  \"Image\": \"%s\",\n"
            "  \"Command\": \"%s\",\n"
            "  \"Created\": %ld,\n"
            "  \"State\": {\n"
            "    \"Status\": \"created\",\n"
            "    \"Running\": false,\n"
            "    \"ExitCode\": 0,\n"
            "    \"FinishedAt\": 0\n"
            "  },\n"
            "  \"RootFS\": \"%s\"\n"
            "}\n",
            out_id, name, image, esc_cmd, (long)now, rootfs);
        fclose(fp);
    }

    return 0;
}

int cm_start_container(const char *storage_root,
                       const char *id,
                       container_run_result_t *res) {
    if (!storage_root || !id || !res) return -1;

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        return -1;
    }

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers/%s", storage_root, real_id);

    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s/metadata.json", c_dir);

    FILE *fp = fopen(meta_path, "r");
    if (!fp) return -1;

    char meta_buf[4096] = {0};
    fread(meta_buf, 1, sizeof(meta_buf) - 1, fp);
    fclose(fp);

    if (strstr(meta_buf, "\"Status\": \"exited\"")) {
        char *pe = strstr(meta_buf, "\"ExitCode\": ");
        if (pe) res->exit_code = atoi(pe + 12);
        return 0;
    }

    char image[256] = "alpine:latest";
    char command[1024] = "echo Container running";
    char name[64] = "container";

    char *p_img = strstr(meta_buf, "\"Image\": \"");
    if (p_img) sscanf(p_img + 10, "%255[^\"]", image);

    char *p_cmd = strstr(meta_buf, "\"Command\": \"");
    if (p_cmd) sscanf(p_cmd + 12, "%1023[^\"]", command);

    char *p_name = strstr(meta_buf, "\"Name\": \"/");
    if (p_name) sscanf(p_name + 10, "%63[^\"]", name);

    /* Dispatch container execution */
    engine_config_t cfg;
    engine_init_defaults(&cfg);
    snprintf(cfg.storage_root, sizeof(cfg.storage_root), "%s", storage_root);

    container_run_spec_t spec;
    memset(&spec, 0, sizeof(spec));
    snprintf(spec.container_id, sizeof(spec.container_id), "%s", real_id);
    snprintf(spec.image_name, sizeof(spec.image_name), "%s", image);
    snprintf(spec.command, sizeof(spec.command), "%s", command);
    spec.timeout_ms = 10000;
    spec.max_mem_mb = 512;

    int rc = engine_dispatch_run(&cfg, &spec, res);

    /* Save stdout and stderr logs */
    char out_path[1024];
    snprintf(out_path, sizeof(out_path), "%s/stdout.log", c_dir);
    FILE *f_out = fopen(out_path, "w");
    if (f_out) {
        if (res->stdout_buf[0]) fputs(res->stdout_buf, f_out);
        fclose(f_out);
    }

    char err_path[1024];
    snprintf(err_path, sizeof(err_path), "%s/stderr.log", c_dir);
    FILE *f_err = fopen(err_path, "w");
    if (f_err) {
        if (res->stderr_buf[0]) fputs(res->stderr_buf, f_err);
        fclose(f_err);
    }

    /* Update metadata state */
    fp = fopen(meta_path, "w");
    if (fp) {
        time_t now = time(NULL);
        char esc_cmd[2048] = {0};
        json_escape_string(command, esc_cmd, sizeof(esc_cmd));
        fprintf(fp,
            "{\n"
            "  \"Id\": \"%s\",\n"
            "  \"Name\": \"/%s\",\n"
            "  \"Image\": \"%s\",\n"
            "  \"Command\": \"%s\",\n"
            "  \"Created\": %ld,\n"
            "  \"State\": {\n"
            "    \"Status\": \"exited\",\n"
            "    \"Running\": false,\n"
            "    \"ExitCode\": %d,\n"
            "    \"FinishedAt\": %ld\n"
            "  },\n"
            "  \"RootFS\": \"%s/rootfs\"\n"
            "}\n",
            real_id, name, image, esc_cmd, (long)now, res->exit_code, (long)now, c_dir);
        fclose(fp);
    }

    return rc;
}

int cm_wait_container(const char *storage_root,
                      const char *id,
                      int *exit_code) {
    if (!storage_root || !id || !exit_code) return -1;

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        *exit_code = 0;
        return 0;
    }

    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s/containers/%s/metadata.json", storage_root, real_id);

    FILE *fp = fopen(meta_path, "r");
    if (!fp) {
        *exit_code = 0;
        return 0;
    }

    char buf[2048] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);

    *exit_code = 0;
    char *p = strstr(buf, "\"ExitCode\": ");
    if (p) {
        *exit_code = atoi(p + 12);
    }
    return 0;
}

int cm_get_logs(const char *storage_root,
                const char *id,
                bool stdout_flag,
                bool stderr_flag,
                bool raw_tty,
                char **out_buf,
                size_t *out_len) {
    if (!storage_root || !id || !out_buf || !out_len) return -1;
    *out_buf = NULL;
    *out_len = 0;

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        return -1;
    }

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers/%s", storage_root, real_id);

    char out_path[1024], err_path[1024];
    snprintf(out_path, sizeof(out_path), "%s/stdout.log", c_dir);
    snprintf(err_path, sizeof(err_path), "%s/stderr.log", c_dir);

    char raw_out[65536] = {0};
    size_t out_bytes = 0;
    if (stdout_flag) {
        FILE *fp = fopen(out_path, "r");
        if (fp) {
            out_bytes = fread(raw_out, 1, sizeof(raw_out) - 1, fp);
            fclose(fp);
        }
    }

    char raw_err[65536] = {0};
    size_t err_bytes = 0;
    if (stderr_flag) {
        FILE *fp = fopen(err_path, "r");
        if (fp) {
            err_bytes = fread(raw_err, 1, sizeof(raw_err) - 1, fp);
            fclose(fp);
        }
    }

    if (raw_tty) {
        /* Raw plain string */
        size_t total = out_bytes + err_bytes;
        char *buf = malloc(total + 1);
        if (!buf) return -1;
        memcpy(buf, raw_out, out_bytes);
        memcpy(buf + out_bytes, raw_err, err_bytes);
        buf[total] = '\0';
        *out_buf = buf;
        *out_len = total;
        return 0;
    }

    /* Docker multiplexed log stream: 8-byte header per chunk */
    size_t total_alloc = (out_bytes > 0 ? (8 + out_bytes) : 0) +
                         (err_bytes > 0 ? (8 + err_bytes) : 0);
    if (total_alloc == 0) {
        *out_buf = strdup("");
        *out_len = 0;
        return 0;
    }

    uint8_t *stream = malloc(total_alloc + 1);
    if (!stream) return -1;

    size_t offset = 0;
    if (out_bytes > 0) {
        stream[offset + 0] = 1; /* 1 = stdout */
        stream[offset + 1] = 0;
        stream[offset + 2] = 0;
        stream[offset + 3] = 0;
        stream[offset + 4] = (uint8_t)((out_bytes >> 24) & 0xFF);
        stream[offset + 5] = (uint8_t)((out_bytes >> 16) & 0xFF);
        stream[offset + 6] = (uint8_t)((out_bytes >> 8) & 0xFF);
        stream[offset + 7] = (uint8_t)(out_bytes & 0xFF);
        memcpy(stream + offset + 8, raw_out, out_bytes);
        offset += 8 + out_bytes;
    }

    if (err_bytes > 0) {
        stream[offset + 0] = 2; /* 2 = stderr */
        stream[offset + 1] = 0;
        stream[offset + 2] = 0;
        stream[offset + 3] = 0;
        stream[offset + 4] = (uint8_t)((err_bytes >> 24) & 0xFF);
        stream[offset + 5] = (uint8_t)((err_bytes >> 16) & 0xFF);
        stream[offset + 6] = (uint8_t)((err_bytes >> 8) & 0xFF);
        stream[offset + 7] = (uint8_t)(err_bytes & 0xFF);
        memcpy(stream + offset + 8, raw_err, err_bytes);
        offset += 8 + err_bytes;
    }

    stream[offset] = '\0';
    *out_buf = (char *)stream;
    *out_len = offset;
    return 0;
}

int cm_inspect_json(const char *storage_root,
                    const char *id,
                    char *out_buf,
                    size_t max_len) {
    if (!storage_root || !id || !out_buf || max_len < 128) return -1;

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        return -1;
    }

    char meta_path[1024];
    snprintf(meta_path, sizeof(meta_path), "%s/containers/%s/metadata.json", storage_root, real_id);

    FILE *fp = fopen(meta_path, "r");
    if (!fp) return -1;

    char buf[4096] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);

    char name[64] = "/container";
    char image[256] = "alpine:latest";
    char command[1024] = "echo";
    int exit_code = 0;
    bool running = false;

    char *p_name = strstr(buf, "\"Name\": \"");
    if (p_name) sscanf(p_name + 9, "%63[^\"]", name);

    char *p_img = strstr(buf, "\"Image\": \"");
    if (p_img) sscanf(p_img + 10, "%255[^\"]", image);

    char *p_cmd = strstr(buf, "\"Command\": \"");
    if (p_cmd) sscanf(p_cmd + 12, "%1023[^\"]", command);

    char *p_exit = strstr(buf, "\"ExitCode\": ");
    if (p_exit) exit_code = atoi(p_exit + 12);

    char esc_cmd[2048] = {0};
    json_escape_string(command, esc_cmd, sizeof(esc_cmd));

    snprintf(out_buf, max_len,
        "{\n"
        "  \"Id\": \"%s\",\n"
        "  \"Created\": \"2026-10-16T12:00:00.000000000Z\",\n"
        "  \"Path\": \"%s\",\n"
        "  \"Args\": [],\n"
        "  \"State\": {\n"
        "    \"Status\": \"%s\",\n"
        "    \"Running\": %s,\n"
        "    \"Paused\": false,\n"
        "    \"Restarting\": false,\n"
        "    \"OOMKilled\": false,\n"
        "    \"Dead\": false,\n"
        "    \"Pid\": 0,\n"
        "    \"ExitCode\": %d,\n"
        "    \"Error\": \"\",\n"
        "    \"StartedAt\": \"2026-10-16T12:00:00.000000000Z\",\n"
        "    \"FinishedAt\": \"2026-10-16T12:00:01.000000000Z\"\n"
        "  },\n"
        "  \"Image\": \"sha256:000000000000\",\n"
        "  \"Name\": \"%s\",\n"
        "  \"HostConfig\": {\"NetworkMode\": \"default\"},\n"
        "  \"Config\": {\"Image\": \"%s\", \"Cmd\": [\"%s\"]}\n"
        "}\n",
        real_id, esc_cmd, running ? "running" : "exited",
        running ? "true" : "false", exit_code, name, image, esc_cmd);

    return 0;
}

int cm_list_containers_json(const char *storage_root,
                            bool all,
                            char *out_buf,
                            size_t max_len) {
    if (!storage_root || !out_buf || max_len < 16) return -1;

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers", storage_root);

    DIR *d = opendir(c_dir);
    if (!d) {
        snprintf(out_buf, max_len, "[]\n");
        return 0;
    }

    size_t offset = 0;
    offset += snprintf(out_buf + offset, max_len - offset, "[\n");

    bool first = true;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;

        char meta_path[1024];
        snprintf(meta_path, sizeof(meta_path), "%s/%s/metadata.json", c_dir, de->d_name);
        FILE *fp = fopen(meta_path, "r");
        if (!fp) continue;

        char buf[4096] = {0};
        fread(buf, 1, sizeof(buf) - 1, fp);
        fclose(fp);

        char name[64] = "/cnt";
        char image[256] = "alpine:latest";
        char command[1024] = "";
        int exit_code = 0;

        char *pn = strstr(buf, "\"Name\": \"");
        if (pn) sscanf(pn + 9, "%63[^\"]", name);

        char *pi = strstr(buf, "\"Image\": \"");
        if (pi) sscanf(pi + 10, "%255[^\"]", image);

        char *pc = strstr(buf, "\"Command\": \"");
        if (pc) sscanf(pc + 12, "%1023[^\"]", command);

        char *pe = strstr(buf, "\"ExitCode\": ");
        if (pe) exit_code = atoi(pe + 12);

        bool is_exited = (strstr(buf, "\"Status\": \"exited\"") != NULL) ||
                         (strstr(buf, "\"Running\": false") != NULL);
        if (!all && is_exited) {
            continue;
        }

        if (!first) {
            offset += snprintf(out_buf + offset, max_len - offset, ",\n");
        }
        first = false;

        char esc_cmd[2048] = {0};
        json_escape_string(command, esc_cmd, sizeof(esc_cmd));

        offset += snprintf(out_buf + offset, max_len - offset,
            "  {\n"
            "    \"Id\": \"%s\",\n"
            "    \"Names\": [\"%s\"],\n"
            "    \"Image\": \"%s\",\n"
            "    \"Command\": \"%s\",\n"
            "    \"Created\": 1792128000,\n"
            "    \"State\": \"exited\",\n"
            "    \"Status\": \"Exited (%d) 1 minute ago\"\n"
            "  }",
            de->d_name, name, image, esc_cmd, exit_code);
    }
    closedir(d);

    if (first) {
        snprintf(out_buf, max_len, "[]\n");
    } else {
        offset += snprintf(out_buf + offset, max_len - offset, "\n]\n");
    }
    return 0;
}

int cm_remove_container(const char *storage_root,
                        const char *id,
                        bool force) {
    (void)force;
    if (!storage_root || !id) return -1;

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        return -1;
    }

    char c_dir[1024];
    snprintf(c_dir, sizeof(c_dir), "%s/containers/%s", storage_root, real_id);

    char cmd[1100];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", c_dir);
    return system(cmd);
}

int cm_stop_container(const char *storage_root,
                      const char *id) {
    (void)storage_root;
    (void)id;
    return 0;
}

int cm_bind_mount(const char *storage_root,
                  const char *id,
                  const char *host_path,
                  const char *container_path) {
    if (!storage_root || !id || !host_path || !container_path) return -1;
    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) return -1;

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/containers/%s/rootfs", storage_root, real_id);

    const char *rel = container_path;
    while (*rel == '/') rel++;

    char target[1024];
    snprintf(target, sizeof(target), "%s/%s", rootfs, rel);

    char dir_cmd[1100];
    snprintf(dir_cmd, sizeof(dir_cmd), "mkdir -p \"$(dirname '%s')\"", target);
    system(dir_cmd);

    char link_cmd[2200];
    snprintf(link_cmd, sizeof(link_cmd), "ln -sfn \"%s\" \"%s\"", host_path, target);
    return system(link_cmd);
}

int cm_copy_to_container(const char *storage_root,
                         const char *id,
                         const char *src_host,
                         const char *dest_container) {
    if (!storage_root || !id || !src_host || !dest_container) return -1;
    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) return -1;

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/containers/%s/rootfs", storage_root, real_id);

    const char *rel = dest_container;
    while (*rel == '/') rel++;

    char target[1024];
    snprintf(target, sizeof(target), "%s/%s", rootfs, rel);

    char cmd[2200];
    struct stat st;
    if (stat(src_host, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" && cp -R \"%s/\"* \"%s/\" 2>/dev/null || cp -R \"%s\" \"%s\"",
                 target, src_host, target, src_host, target);
    } else {
        snprintf(cmd, sizeof(cmd), "mkdir -p \"$(dirname '%s')\" && cp -f \"%s\" \"%s\"", target, src_host, target);
    }
    return system(cmd);
}

int cm_copy_from_container(const char *storage_root,
                           const char *id,
                           const char *src_container,
                           const char *dest_host) {
    if (!storage_root || !id || !src_container || !dest_host) return -1;
    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) return -1;

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/containers/%s/rootfs", storage_root, real_id);

    const char *rel = src_container;
    while (*rel == '/') rel++;

    char src[1024];
    snprintf(src, sizeof(src), "%s/%s", rootfs, rel);

    char cmd[2200];
    struct stat st;
    if (stat(src, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\" && cp -R \"%s/\"* \"%s/\" 2>/dev/null || cp -R \"%s\" \"%s\"",
                 dest_host, src, dest_host, src, dest_host);
    } else {
        snprintf(cmd, sizeof(cmd), "mkdir -p \"$(dirname '%s')\" && cp -f \"%s\" \"%s\"", dest_host, src, dest_host);
    }
    return system(cmd);
}

int cm_extract_tar_to_container(const char *storage_root,
                                const char *id,
                                const char *target_dir,
                                const char *tar_data,
                                size_t tar_len) {
    if (!storage_root || !id || !tar_data || tar_len == 0) return -1;
    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) return -1;

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/containers/%s/rootfs", storage_root, real_id);

    const char *rel = target_dir ? target_dir : "/";
    while (*rel == '/') rel++;

    char target[1024];
    if (rel[0]) {
        snprintf(target, sizeof(target), "%s/%s", rootfs, rel);
    } else {
        snprintf(target, sizeof(target), "%s", rootfs);
    }

    char dir_cmd[1100];
    snprintf(dir_cmd, sizeof(dir_cmd), "mkdir -p \"%s\"", target);
    system(dir_cmd);

    char tar_path[1024];
    snprintf(tar_path, sizeof(tar_path), "/tmp/upload_%d.tar", getpid());
    FILE *fp = fopen(tar_path, "wb");
    if (!fp) return -1;
    fwrite(tar_data, 1, tar_len, fp);
    fclose(fp);

    char extract_cmd[2200];
    snprintf(extract_cmd, sizeof(extract_cmd), "tar -xf \"%s\" -C \"%s\" 2>/dev/null", tar_path, target);
    int rc = system(extract_cmd);
    unlink(tar_path);
    return rc;
}

