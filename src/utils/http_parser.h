/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · High-Performance HTTP Request Parser
 */

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "docker_api.h"

/* Parses raw HTTP buffer into structured http_request_t */
int http_parse_request(const char *raw_buf, size_t raw_len, http_request_t *req);

/* Formats and sends HTTP response back to client socket */
int http_send_response(int fd, const http_response_t *resp);

/* Helper to strip OCI/Docker version prefix, e.g., /v1.45/_ping -> /_ping */
void http_normalize_docker_path(const char *raw_path, char *clean_path, size_t max_len);

#endif /* HTTP_PARSER_H */
