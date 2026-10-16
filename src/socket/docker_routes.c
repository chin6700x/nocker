/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Docker REST API Handlers Implementation
 */

#include "docker_api.h"
#include "container_manager.h"
#include "oci_spec.h"
#include <sys/sysctl.h>

void docker_response_free(http_response_t *resp) {
    if (resp && resp->body) {
        free(resp->body);
        resp->body = NULL;
    }
}

static void get_storage_root(char *buf, size_t len) {
    const char *home = getenv("HOME");
    snprintf(buf, len, "%s/.nocker/storage", home ? home : "/tmp");
}

static void extract_container_id(const char *path, char *out_id, size_t len) {
    out_id[0] = '\0';
    if (!path || strncmp(path, "/containers/", 12) != 0) return;
    const char *p = path + 12;
    size_t i = 0;
    while (*p && *p != '/' && *p != '?' && i + 1 < len) {
        out_id[i++] = *p++;
    }
    out_id[i] = '\0';
}

int handle_docker_ping(const http_request_t *req, http_response_t *resp) {
    (void)req;
    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "text/plain; charset=utf-8");
    resp->body = strdup("OK");
    resp->body_len = 2;
    return 0;
}

int handle_docker_version(const http_request_t *req, http_response_t *resp) {
    (void)req;
    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");

    const char *ver_json =
        "{\n"
        "  \"Platform\": {\"Name\": \"nocker/darwin-arm64\"},\n"
        "  \"Components\": [\n"
        "    {\n"
        "      \"Name\": \"Engine\",\n"
        "      \"Version\": \"" DOCKER_ENGINE_VER "\",\n"
        "      \"Details\": {\n"
        "        \"ApiVersion\": \"" DOCKER_API_VERSION "\",\n"
        "        \"Arch\": \"arm64\",\n"
        "        \"BuildTime\": \"2026-10-16T11:00:00.000000000+00:00\",\n"
        "        \"Experimental\": \"false\",\n"
        "        \"GitCommit\": \"" GIT_COMMIT "\",\n"
        "        \"GoVersion\": \"go1.22.0\",\n"
        "        \"KernelVersion\": \"24.0.0\",\n"
        "        \"MinAPIVersion\": \"1.12\",\n"
        "        \"Os\": \"darwin\"\n"
        "      }\n"
        "    }\n"
        "  ],\n"
        "  \"Version\": \"" DOCKER_ENGINE_VER "\",\n"
        "  \"ApiVersion\": \"" DOCKER_API_VERSION "\",\n"
        "  \"MinAPIVersion\": \"1.12\",\n"
        "  \"GitCommit\": \"" GIT_COMMIT "\",\n"
        "  \"GoVersion\": \"go1.22.0\",\n"
        "  \"Os\": \"darwin\",\n"
        "  \"Arch\": \"arm64\",\n"
        "  \"KernelVersion\": \"24.0.0\",\n"
        "  \"BuildTime\": \"2026-10-16T11:00:00.000000000+00:00\"\n"
        "}\n";

    resp->body = strdup(ver_json);
    resp->body_len = strlen(resp->body);
    return 0;
}

int handle_docker_info(const http_request_t *req, http_response_t *resp) {
    (void)req;
    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");

    /* Detect CPU Cores and RAM */
    int ncpu = 8;
    size_t len = sizeof(ncpu);
    sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0);

    uint64_t mem_bytes = 17179869184ULL;
    len = sizeof(mem_bytes);
    sysctlbyname("hw.memsize", &mem_bytes, &len, NULL, 0);

    char buf[4096];
    snprintf(buf, sizeof(buf),
        "{\n"
        "  \"ID\": \"NOCKER-2026-ZEPHYR\",\n"
        "  \"Containers\": 0,\n"
        "  \"ContainersRunning\": 0,\n"
        "  \"ContainersPaused\": 0,\n"
        "  \"ContainersStopped\": 0,\n"
        "  \"Images\": 0,\n"
        "  \"Driver\": \"apfs-cow\",\n"
        "  \"DriverStatus\": [[\"Backing Filesystem\", \"apfs\"], [\"CoW Snapshotter\", \"clonefile\"]],\n"
        "  \"Plugins\": {\"Volume\": [\"local\"], \"Network\": [\"bridge\", \"host\"], \"Log\": [\"json-file\"]},\n"
        "  \"MemoryLimit\": true,\n"
        "  \"SwapLimit\": false,\n"
        "  \"CPUCfsPeriod\": true,\n"
        "  \"CPUCfsQuota\": true,\n"
        "  \"PidsLimit\": true,\n"
        "  \"IPv4Forwarding\": true,\n"
        "  \"KernelVersion\": \"24.0.0\",\n"
        "  \"OperatingSystem\": \"macOS Darwin (Apple Silicon Native)\",\n"
        "  \"OSType\": \"darwin\",\n"
        "  \"Architecture\": \"arm64\",\n"
        "  \"IndexServerAddress\": \"https://index.docker.io/v1/\",\n"
        "  \"NCPU\": %d,\n"
        "  \"MemTotal\": %llu,\n"
        "  \"Name\": \"nocker-silicon\",\n"
        "  \"ServerVersion\": \"" DOCKER_ENGINE_VER "\",\n"
        "  \"Runtimes\": {\n"
        "    \"vz\": {\"path\": \"apple-virtualization\"},\n"
        "    \"seatbelt\": {\"path\": \"darwin-seatbelt\"},\n"
        "    \"crun\": {\"path\": \"crun\"}\n"
        "  },\n"
        "  \"DefaultRuntime\": \"vz\",\n"
        "  \"Isolation\": \"hypervisor\",\n"
        "  \"SecurityOptions\": [\"name=seatbelt\"]\n"
        "}\n",
        ncpu, (unsigned long long)mem_bytes);

    resp->body = strdup(buf);
    resp->body_len = strlen(resp->body);
    return 0;
}

int handle_docker_containers_json(const http_request_t *req, http_response_t *resp) {
    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");

    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    bool all = (strstr(req->query, "all=1") != NULL) || (strstr(req->query, "all=true") != NULL);
    char buf[65536];
    if (cm_list_containers_json(storage_root, all, buf, sizeof(buf)) == 0) {
        resp->body = strdup(buf);
    } else {
        resp->body = strdup("[]\n");
    }
    resp->body_len = strlen(resp->body);
    return 0;
}

static void json_unescape(const char *src, size_t src_len, char *dst, size_t dst_max) {
    size_t d = 0;
    for (size_t s = 0; s < src_len && d + 1 < dst_max; s++) {
        if (src[s] == '\\' && s + 1 < src_len) {
            s++;
            if (src[s] == 'u' && s + 4 < src_len) {
                if (strncmp(src + s, "u0026", 5) == 0) {
                    dst[d++] = '&';
                    s += 4;
                    continue;
                }
                if (strncmp(src + s, "u003c", 5) == 0) {
                    dst[d++] = '<';
                    s += 4;
                    continue;
                }
                if (strncmp(src + s, "u003e", 5) == 0) {
                    dst[d++] = '>';
                    s += 4;
                    continue;
                }
            } else if (src[s] == 'n') {
                dst[d++] = '\n';
            } else if (src[s] == 't') {
                dst[d++] = '\t';
            } else if (src[s] == 'r') {
                dst[d++] = '\r';
            } else if (src[s] == '"' || src[s] == '\\' || src[s] == '/') {
                dst[d++] = src[s];
            } else {
                dst[d++] = src[s];
            }
        } else {
            dst[d++] = src[s];
        }
    }
    dst[d] = '\0';
}

static void shell_quote_arg(const char *in, char *out, size_t out_max) {
    if (!in || !out || out_max < 3) return;
    bool needs_quotes = false;
    for (size_t i = 0; in[i]; i++) {
        if (in[i] == ' ' || in[i] == '\t' || in[i] == ';' || in[i] == '&' ||
            in[i] == '|' || in[i] == '<' || in[i] == '>' || in[i] == '$' ||
            in[i] == '(' || in[i] == ')' || in[i] == '`' || in[i] == '"' || in[i] == '\'') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) {
        snprintf(out, out_max, "%s", in);
        return;
    }
    size_t o = 0;
    out[o++] = '\'';
    for (size_t i = 0; in[i] && o + 6 < out_max; i++) {
        if (in[i] == '\'') {
            out[o++] = '\'';
            out[o++] = '\\';
            out[o++] = '\'';
            out[o++] = '\'';
        } else {
            out[o++] = in[i];
        }
    }
    out[o++] = '\'';
    out[o] = '\0';
}

int handle_docker_containers_create(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char image[256] = "alpine:latest";
    char cmd_str[1024] = "";
    char name_hint[64] = "";

    if (req->query[0]) {
        const char *np = strstr(req->query, "name=");
        if (np) {
            sscanf(np + 5, "%63[^& \r\n]", name_hint);
        }
    }

    if (req->body && req->body[0]) {
        const char *p_img = strstr(req->body, "\"Image\"");
        if (p_img) {
            const char *colon = strchr(p_img, ':');
            if (colon) {
                const char *q1 = strchr(colon, '"');
                if (q1) {
                    const char *q2 = strchr(q1 + 1, '"');
                    if (q2 && (size_t)(q2 - (q1 + 1)) < sizeof(image)) {
                        memcpy(image, q1 + 1, q2 - (q1 + 1));
                        image[q2 - (q1 + 1)] = '\0';
                    }
                }
            }
        }

        const char *p_cmd = strstr(req->body, "\"Cmd\"");
        if (p_cmd) {
            const char *colon = strchr(p_cmd, ':');
            if (colon) {
                const char *open_sq = strchr(colon, '[');
                const char *close_sq = open_sq ? strchr(open_sq, ']') : NULL;
                if (open_sq && close_sq && open_sq < close_sq) {
                    const char *cur = open_sq + 1;
                    while (cur < close_sq) {
                        const char *q1 = strchr(cur, '"');
                        if (!q1 || q1 >= close_sq) break;
                        const char *q2 = strchr(q1 + 1, '"');
                        if (!q2 || q2 > close_sq) break;
                        char raw_arg[512] = {0};
                        size_t alen = (size_t)(q2 - (q1 + 1));
                        if (alen >= sizeof(raw_arg)) alen = sizeof(raw_arg) - 1;
                        memcpy(raw_arg, q1 + 1, alen);
                        char unesc[512] = {0};
                        json_unescape(raw_arg, strlen(raw_arg), unesc, sizeof(unesc));
                        char quoted[1024] = {0};
                        shell_quote_arg(unesc, quoted, sizeof(quoted));
                        if (cmd_str[0]) strncat(cmd_str, " ", sizeof(cmd_str) - strlen(cmd_str) - 1);
                        strncat(cmd_str, quoted, sizeof(cmd_str) - strlen(cmd_str) - 1);
                        cur = q2 + 1;
                    }
                }
            }
        }
    }

    char out_id[65] = {0};
    int rc = cm_create_container(storage_root, image, cmd_str, name_hint, out_id, sizeof(out_id));

    if (rc == 0 && req->body) {
        const char *p_binds = strstr(req->body, "\"Binds\"");
        if (p_binds) {
            const char *open_sq = strchr(p_binds, '[');
            const char *close_sq = open_sq ? strchr(open_sq, ']') : NULL;
            if (open_sq && close_sq) {
                const char *cur = open_sq + 1;
                while (cur < close_sq) {
                    const char *q1 = strchr(cur, '"');
                    if (!q1 || q1 >= close_sq) break;
                    const char *q2 = strchr(q1 + 1, '"');
                    if (!q2 || q2 > close_sq) break;
                    char bstr[1024] = {0};
                    size_t blen = (size_t)(q2 - (q1 + 1));
                    if (blen >= sizeof(bstr)) blen = sizeof(bstr) - 1;
                    memcpy(bstr, q1 + 1, blen);
                    char *col = strchr(bstr, ':');
                    if (col) {
                        *col = '\0';
                        char *col2 = strchr(col + 1, ':');
                        if (col2) *col2 = '\0';
                        cm_bind_mount(storage_root, out_id, bstr, col + 1);
                    }
                    cur = q2 + 1;
                }
            }
        }
    }

    if (rc == 0) {
        resp->status_code = 201;
        resp->status_text = "Created";
        snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
        char json[256];
        snprintf(json, sizeof(json), "{\"Id\":\"%s\",\"Warnings\":[]}\n", out_id);
        resp->body = strdup(json);
        resp->body_len = strlen(resp->body);
    } else {
        resp->status_code = 500;
        resp->status_text = "Internal Server Error";
        snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
        resp->body = strdup("{\"message\":\"Failed to create container\"}\n");
        resp->body_len = strlen(resp->body);
    }
    return 0;
}

int handle_docker_containers_start(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    container_run_result_t res;
    cm_start_container(storage_root, id, &res);

    resp->status_code = 204;
    resp->status_text = "No Content";
    resp->body = strdup("");
    resp->body_len = 0;
    return 0;
}

int handle_docker_containers_wait(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    int exit_code = 0;
    cm_wait_container(storage_root, id, &exit_code);

    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
    char json[128];
    snprintf(json, sizeof(json), "{\"StatusCode\":%d,\"Error\":null}\n", exit_code);
    resp->body = strdup(json);
    resp->body_len = strlen(resp->body);
    return 0;
}

int handle_docker_containers_logs(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    bool stdout_flag = (strstr(req->query, "stdout=1") != NULL) || (strstr(req->query, "stdout=true") != NULL);
    bool stderr_flag = (strstr(req->query, "stderr=1") != NULL) || (strstr(req->query, "stderr=true") != NULL);
    if (!stdout_flag && !stderr_flag) {
        stdout_flag = true;
        stderr_flag = true;
    }

    bool raw_tty = (strstr(req->query, "raw=1") != NULL) ||
                   (strstr(req->query, "plain=1") != NULL);

    char *buf = NULL;
    size_t len = 0;
    cm_get_logs(storage_root, id, stdout_flag, stderr_flag, raw_tty, &buf, &len);

    resp->status_code = 200;
    resp->status_text = "OK";
    if (raw_tty) {
        snprintf(resp->content_type, sizeof(resp->content_type), "text/plain; charset=utf-8");
    } else {
        snprintf(resp->content_type, sizeof(resp->content_type), "application/vnd.docker.raw-stream");
    }
    resp->body = buf ? buf : strdup("");
    resp->body_len = len;
    return 0;
}

int handle_docker_containers_attach(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    resp->status_code = 101;
    resp->status_text = "UPGRADED";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/vnd.docker.raw-stream");

    /* If container has not run yet (status created), start it now for the attached stream */
    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) == 0) {
        char meta_path[1024];
        snprintf(meta_path, sizeof(meta_path), "%s/containers/%s/metadata.json", storage_root, real_id);
        FILE *fp = fopen(meta_path, "r");
        if (fp) {
            char buf[2048] = {0};
            fread(buf, 1, sizeof(buf) - 1, fp);
            fclose(fp);
            if (strstr(buf, "\"Status\": \"created\"")) {
                container_run_result_t res;
                cm_start_container(storage_root, real_id, &res);
            }
        }
    }

    char *buf = NULL;
    size_t len = 0;
    cm_get_logs(storage_root, id, true, true, false, &buf, &len);

    resp->body = buf ? buf : strdup("");
    resp->body_len = len;
    return 0;
}

int handle_docker_containers_inspect(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    char buf[4096];
    if (cm_inspect_json(storage_root, id, buf, sizeof(buf)) == 0) {
        resp->status_code = 200;
        resp->status_text = "OK";
        snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
        resp->body = strdup(buf);
        resp->body_len = strlen(resp->body);
    } else {
        resp->status_code = 404;
        resp->status_text = "Not Found";
        snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
        resp->body = strdup("{\"message\":\"No such container\"}\n");
        resp->body_len = strlen(resp->body);
    }
    return 0;
}

int handle_docker_containers_delete(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    bool force = (strstr(req->query, "force=1") != NULL) || (strstr(req->query, "force=true") != NULL);
    cm_remove_container(storage_root, id, force);

    resp->status_code = 204;
    resp->status_text = "No Content";
    resp->body = strdup("");
    resp->body_len = 0;
    return 0;
}

int handle_docker_containers_stop(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    cm_stop_container(storage_root, id);

    resp->status_code = 204;
    resp->status_text = "No Content";
    resp->body = strdup("");
    resp->body_len = 0;
    return 0;
}

int handle_docker_images_json(const http_request_t *req, http_response_t *resp) {
    (void)req;
    resp->status_code = 200;
    resp->status_text = "OK";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");

    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char buf[65536];
    if (oci_list_images_json(storage_root, buf, sizeof(buf)) == 0) {
        resp->body = strdup(buf);
    } else {
        resp->body = strdup("[]\n");
    }
    resp->body_len = strlen(resp->body);
    return 0;
}

int handle_docker_images_create(const http_request_t *req, http_response_t *resp) {
    char from_img[256] = "alpine";
    char tag[64] = "latest";

    if (req->query[0]) {
        const char *p = strstr(req->query, "fromImage=");
        if (p) {
            sscanf(p + 10, "%255[^& \r\n]", from_img);
        }
        const char *t = strstr(req->query, "tag=");
        if (t) {
            sscanf(t + 4, "%63[^& \r\n]", tag);
        }
    }

    char full_ref[512];
    snprintf(full_ref, sizeof(full_ref), "%s:%s", from_img, tag);

    oci_image_ref_t oci_ref;
    oci_parse_image_ref(full_ref, &oci_ref);

    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    oci_image_metadata_t meta;
    int rc = oci_pull_image(&oci_ref, storage_root, &meta);

    resp->status_code = (rc == 0) ? 200 : 500;
    resp->status_text = (rc == 0) ? "OK" : "Internal Server Error";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");

    char res_buf[512];
    if (rc == 0) {
        snprintf(res_buf, sizeof(res_buf),
                 "{\"status\":\"Download complete\",\"id\":\"%s\"}\n", meta.id);
    } else {
        snprintf(res_buf, sizeof(res_buf),
                 "{\"message\":\"Failed to pull image %s\"}\n", full_ref);
    }
    resp->body = strdup(res_buf);
    resp->body_len = strlen(resp->body);
    return 0;
}

int handle_docker_containers_archive_put(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    char target_path[1024] = "/";
    if (req->query[0]) {
        const char *p = strstr(req->query, "path=");
        if (p) {
            sscanf(p + 5, "%1023[^& \r\n]", target_path);
        }
    }

    int rc = cm_extract_tar_to_container(storage_root, id, target_path, req->body, req->content_length);
    if (rc == 0) {
        resp->status_code = 200;
        resp->status_text = "OK";
        resp->body = strdup("");
        resp->body_len = 0;
    } else {
        resp->status_code = 500;
        resp->status_text = "Internal Server Error";
        resp->body = strdup("{\"message\":\"Failed to extract archive\"}\n");
        resp->body_len = strlen(resp->body);
    }
    return 0;
}

int handle_docker_containers_archive_get(const http_request_t *req, http_response_t *resp) {
    char storage_root[1024];
    get_storage_root(storage_root, sizeof(storage_root));

    char id[65] = {0};
    extract_container_id(req->path, id, sizeof(id));

    char real_id[65];
    if (cm_resolve_id(storage_root, id, real_id, sizeof(real_id)) != 0) {
        resp->status_code = 404;
        resp->status_text = "Not Found";
        return 0;
    }

    char src_path[1024] = "/";
    if (req->query[0]) {
        const char *p = strstr(req->query, "path=");
        if (p) {
            sscanf(p + 5, "%1023[^& \r\n]", src_path);
        }
    }

    char rootfs[1024];
    snprintf(rootfs, sizeof(rootfs), "%s/containers/%s/rootfs", storage_root, real_id);
    const char *rel = src_path;
    while (*rel == '/') rel++;
    char full[1024];
    snprintf(full, sizeof(full), "%s/%s", rootfs, rel);

    char tar_path[1024];
    snprintf(tar_path, sizeof(tar_path), "/tmp/down_%d.tar", getpid());
    char cmd[2200];
    snprintf(cmd, sizeof(cmd), "tar -cf \"%s\" -C \"$(dirname '%s')\" \"$(basename '%s')\" 2>/dev/null", tar_path, full, full);
    system(cmd);

    FILE *fp = fopen(tar_path, "rb");
    if (fp) {
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        char *buf = malloc(sz + 1);
        if (buf) {
            fread(buf, 1, sz, fp);
            resp->status_code = 200;
            resp->status_text = "OK";
            snprintf(resp->content_type, sizeof(resp->content_type), "application/x-tar");
            resp->body = buf;
            resp->body_len = sz;
        }
        fclose(fp);
        unlink(tar_path);
    } else {
        resp->status_code = 404;
        resp->status_text = "Not Found";
    }
    return 0;
}

int docker_dispatch_request(const http_request_t *req, http_response_t *resp) {
    if (!req || !resp) return -1;
    memset(resp, 0, sizeof(http_response_t));

    /* Root health route */
    if (strcmp(req->path, "/") == 0) {
        return handle_docker_ping(req, resp);
    }

    /* Route dispatching */
    if (strcmp(req->path, "/_ping") == 0) {
        return handle_docker_ping(req, resp);
    }
    if (strcmp(req->path, "/version") == 0) {
        return handle_docker_version(req, resp);
    }
    if (strcmp(req->path, "/info") == 0) {
        return handle_docker_info(req, resp);
    }
    if (strcmp(req->path, "/containers/json") == 0) {
        return handle_docker_containers_json(req, resp);
    }
    if (strcmp(req->path, "/containers/create") == 0 && strcmp(req->method, "POST") == 0) {
        return handle_docker_containers_create(req, resp);
    }
    if (strncmp(req->path, "/containers/", 12) == 0) {
        const char *sub = req->path + 12;
        if (strstr(sub, "/archive")) {
            if (strcmp(req->method, "PUT") == 0) {
                return handle_docker_containers_archive_put(req, resp);
            }
            if (strcmp(req->method, "GET") == 0) {
                return handle_docker_containers_archive_get(req, resp);
            }
        }
        if (strstr(sub, "/start") && strcmp(req->method, "POST") == 0) {
            return handle_docker_containers_start(req, resp);
        }
        if (strstr(sub, "/wait") && strcmp(req->method, "POST") == 0) {
            return handle_docker_containers_wait(req, resp);
        }
        if (strstr(sub, "/logs") && strcmp(req->method, "GET") == 0) {
            return handle_docker_containers_logs(req, resp);
        }
        if (strstr(sub, "/attach") && strcmp(req->method, "POST") == 0) {
            return handle_docker_containers_attach(req, resp);
        }
        if (strstr(sub, "/json") && strcmp(req->method, "GET") == 0) {
            return handle_docker_containers_inspect(req, resp);
        }
        if (strstr(sub, "/stop") && strcmp(req->method, "POST") == 0) {
            return handle_docker_containers_stop(req, resp);
        }
        if (strcmp(req->method, "DELETE") == 0) {
            return handle_docker_containers_delete(req, resp);
        }
        if (strcmp(req->method, "GET") == 0 && !strchr(sub, '/')) {
            return handle_docker_containers_inspect(req, resp);
        }
    }
    if (strcmp(req->path, "/images/json") == 0) {
        return handle_docker_images_json(req, resp);
    }
    if (strcmp(req->path, "/images/create") == 0) {
        return handle_docker_images_create(req, resp);
    }
    if (strcmp(req->path, "/containers/prune") == 0 || strcmp(req->path, "/images/prune") == 0) {
        resp->status_code = 200;
        resp->status_text = "OK";
        snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
        resp->body = strdup("{\"ContainersDeleted\":[],\"SpaceReclaimed\":0}\n");
        resp->body_len = strlen(resp->body);
        return 0;
    }

    /* Fallback: 404 Not Found */
    resp->status_code = 404;
    resp->status_text = "Not Found";
    snprintf(resp->content_type, sizeof(resp->content_type), "application/json");
    char err_buf[256];
    snprintf(err_buf, sizeof(err_buf), "{\"message\": \"page not found: %s\"}\n", req->path);
    resp->body = strdup(err_buf);
    resp->body_len = strlen(resp->body);
    return 0;
}
