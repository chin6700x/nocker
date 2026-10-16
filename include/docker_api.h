/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Docker REST API v1.45 Specifications & Handlers
 */

#ifndef DOCKER_API_H
#define DOCKER_API_H

#include "nocker.h"

typedef struct {
    char method[16];        /* GET, POST, DELETE, PUT, HEAD */
    char path[1024];        /* e.g., /_ping, /version, /containers/json */
    char query[1024];       /* query string */
    char content_type[128];
    size_t content_length;
    char *body;
} http_request_t;

typedef struct {
    int status_code;
    const char *status_text;
    char content_type[128];
    char *body;
    size_t body_len;
} http_response_t;

/* HTTP & Docker Request Dispatcher */
int docker_dispatch_request(const http_request_t *req, http_response_t *resp);
void docker_response_free(http_response_t *resp);

/* Core Endpoint Handlers */
int handle_docker_ping(const http_request_t *req, http_response_t *resp);
int handle_docker_version(const http_request_t *req, http_response_t *resp);
int handle_docker_info(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_json(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_create(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_start(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_wait(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_logs(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_attach(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_inspect(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_delete(const http_request_t *req, http_response_t *resp);
int handle_docker_containers_stop(const http_request_t *req, http_response_t *resp);
int handle_docker_images_json(const http_request_t *req, http_response_t *resp);
int handle_docker_images_create(const http_request_t *req, http_response_t *resp);

#endif /* DOCKER_API_H */
