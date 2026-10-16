/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · High-Performance HTTP Request Parser Implementation
 */

#include "http_parser.h"
#include <ctype.h>

void http_normalize_docker_path(const char *raw_path, char *clean_path, size_t max_len) {
    if (!raw_path || !clean_path || max_len == 0) return;

    /* Check if starts with /v1.XX/ */
    if (raw_path[0] == '/' && raw_path[1] == 'v' && isdigit((unsigned char)raw_path[2])) {
        const char *slash = strchr(raw_path + 2, '/');
        if (slash) {
            snprintf(clean_path, max_len, "%s", slash);
            return;
        }
    }
    snprintf(clean_path, max_len, "%s", raw_path);
}

int http_parse_request(const char *raw_buf, size_t raw_len, http_request_t *req) {
    if (!raw_buf || raw_len == 0 || !req) return -1;
    memset(req, 0, sizeof(http_request_t));

    /* 1. Parse Request Line: METHOD PATH HTTP/X.X */
    const char *p = raw_buf;
    const char *line_end = strstr(p, "\r\n");
    if (!line_end) line_end = strchr(p, '\n');
    if (!line_end) return -1;

    char req_line[2048];
    size_t line_len = (size_t)(line_end - p);
    if (line_len >= sizeof(req_line)) line_len = sizeof(req_line) - 1;
    memcpy(req_line, p, line_len);
    req_line[line_len] = '\0';

    char raw_path[1024] = {0};
    char proto[32] = {0};
    if (sscanf(req_line, "%15s %1023s %31s", req->method, raw_path, proto) < 2) {
        return -1;
    }

    /* Split query string from path */
    char *q = strchr(raw_path, '?');
    if (q) {
        *q = '\0';
        snprintf(req->query, sizeof(req->query), "%s", q + 1);
    }
    http_normalize_docker_path(raw_path, req->path, sizeof(req->path));

    /* 2. Find Body Boundary (\r\n\r\n or \n\n) */
    const char *body_start = strstr(raw_buf, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
    } else {
        body_start = strstr(raw_buf, "\n\n");
        if (body_start) body_start += 2;
    }

    if (body_start && (size_t)(body_start - raw_buf) <= raw_len) {
        req->body = (char *)body_start;
        req->content_length = raw_len - (size_t)(body_start - raw_buf);
    }

    /* 3. Extract Content-Length & Content-Type from headers */
    const char *hdr_len = strcasestr(raw_buf, "Content-Length:");
    if (hdr_len && (!body_start || hdr_len < body_start)) {
        req->content_length = (size_t)strtoul(hdr_len + 15, NULL, 10);
    }

    const char *hdr_type = strcasestr(raw_buf, "Content-Type:");
    if (hdr_type && (!body_start || hdr_type < body_start)) {
        sscanf(hdr_type + 13, " %127[^\r\n]", req->content_type);
    }

    return 0;
}

int http_send_response(int fd, const http_response_t *resp) {
    if (fd < 0 || !resp) return -1;

    size_t body_len = resp->body ? (resp->body_len > 0 ? resp->body_len : strlen(resp->body)) : 0;

    if (resp->status_code == 101) {
        char header_buf[512];
        int hlen = snprintf(header_buf, sizeof(header_buf),
                            "HTTP/1.1 101 UPGRADED\r\n"
                            "Content-Type: %s\r\n"
                            "Connection: Upgrade\r\n"
                            "Upgrade: tcp\r\n\r\n",
                            resp->content_type[0] ? resp->content_type : "application/vnd.docker.raw-stream");
        write(fd, header_buf, (size_t)hlen);
        if (resp->body && body_len > 0) {
            write(fd, resp->body, body_len);
        }
        return 0;
    }

    const char *status_text = resp->status_text ? resp->status_text : "OK";
    const char *content_type = resp->content_type[0] ? resp->content_type : "application/json";

    char header_buf[1024];
    int hlen = snprintf(header_buf, sizeof(header_buf),
                        "HTTP/1.1 %d %s\r\n"
                        "Server: nocker/" ENGINE_VERSION "\r\n"
                        "Api-Version: " DOCKER_API_VERSION "\r\n"
                        "Docker-Experimental: false\r\n"
                        "OSType: darwin\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %zu\r\n"
                        "Connection: close\r\n\r\n",
                        resp->status_code, status_text,
                        content_type, body_len);

    if (hlen > 0) {
        write(fd, header_buf, (size_t)hlen);
    }
    if (resp->body && body_len > 0) {
        write(fd, resp->body, body_len);
    }
    return 0;
}
