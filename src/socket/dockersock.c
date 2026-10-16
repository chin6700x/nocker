/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · UNIX Domain Socket & TCP REST Server
 */

#include "nocker.h"
#include "docker_api.h"
#include "http_parser.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

static volatile sig_atomic_t g_running = 1;

static void sig_handler(int sig) {
    (void)sig;
    g_running = 0;
}

static void ensure_parent_dirs(const char *path) {
    char temp[1024];
    snprintf(temp, sizeof(temp), "%s", path);
    char *p = strrchr(temp, '/');
    if (p) {
        *p = '\0';
        char cmd[1100];
        snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", temp);
        (void)system(cmd);
    }
}

static void handle_client(int client_fd) {
    char req_buf[65536];
    size_t total_read = 0;

    /* 1. Read until headers complete */
    while (total_read < sizeof(req_buf) - 1) {
        ssize_t n = read(client_fd, req_buf + total_read, sizeof(req_buf) - 1 - total_read);
        if (n <= 0) break;
        total_read += (size_t)n;
        req_buf[total_read] = '\0';
        if (strstr(req_buf, "\r\n\r\n") || strstr(req_buf, "\n\n")) {
            break;
        }
    }
    if (total_read == 0) {
        close(client_fd);
        return;
    }

    /* 2. Read remaining body if Content-Length specified */
    const char *hdr = strcasestr(req_buf, "Content-Length:");
    if (hdr) {
        size_t content_len = (size_t)strtoul(hdr + 15, NULL, 10);
        const char *body_start = strstr(req_buf, "\r\n\r\n");
        if (body_start) {
            body_start += 4;
        } else {
            body_start = strstr(req_buf, "\n\n");
            if (body_start) body_start += 2;
        }
        if (body_start) {
            size_t body_already = total_read - (size_t)(body_start - req_buf);
            while (body_already < content_len && total_read < sizeof(req_buf) - 1) {
                ssize_t n = read(client_fd, req_buf + total_read, sizeof(req_buf) - 1 - total_read);
                if (n <= 0) break;
                total_read += (size_t)n;
                body_already += (size_t)n;
                req_buf[total_read] = '\0';
            }
        }
    }

    http_request_t req;
    if (http_parse_request(req_buf, total_read, &req) == 0) {
        http_response_t resp;
        docker_dispatch_request(&req, &resp);
        http_send_response(client_fd, &resp);
        docker_response_free(&resp);
    } else {
        const char *bad = "HTTP/1.1 400 Bad Request\r\nContent-Length: 15\r\nConnection: close\r\n\r\nBad HTTP Syntax";
        write(client_fd, bad, strlen(bad));
    }
    close(client_fd);
}

int engine_run_daemon(const engine_config_t *cfg) {
    if (!cfg || !cfg->socket_path[0]) return -1;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGPIPE, SIG_IGN);

    ensure_parent_dirs(cfg->socket_path);
    unlink(cfg->socket_path);

    int unix_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_fd < 0) {
        perror("Failed to create UNIX domain socket");
        return -1;
    }

    struct sockaddr_un un_addr;
    memset(&un_addr, 0, sizeof(un_addr));
    un_addr.sun_family = AF_UNIX;
    strncpy(un_addr.sun_path, cfg->socket_path, sizeof(un_addr.sun_path) - 1);

    if (bind(unix_fd, (struct sockaddr *)&un_addr, sizeof(un_addr)) < 0) {
        perror("Failed to bind UNIX domain socket");
        close(unix_fd);
        return -1;
    }

    /* Set socket permissions to allow developer tools access */
    chmod(cfg->socket_path, 0666);

    if (listen(unix_fd, 64) < 0) {
        perror("Failed to listen on UNIX socket");
        close(unix_fd);
        unlink(cfg->socket_path);
        return -1;
    }

    /* Optional TCP Server for Remote/Web Studio */
    int tcp_fd = -1;
    if (cfg->port > 0) {
        tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (tcp_fd >= 0) {
            int opt = 1;
            setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            struct sockaddr_in sin;
            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;
            sin.sin_port = htons((uint16_t)cfg->port);
            sin.sin_addr.s_addr = INADDR_ANY;
            if (bind(tcp_fd, (struct sockaddr *)&sin, sizeof(sin)) == 0) {
                listen(tcp_fd, 32);
            } else {
                close(tcp_fd);
                tcp_fd = -1;
            }
        }
    }

    /* Also link docker.sock in the same directory for seamless compatibility */
    char compat_sock[1024];
    snprintf(compat_sock, sizeof(compat_sock), "%s", cfg->socket_path);
    char *last_slash = strrchr(compat_sock, '/');
    if (last_slash) {
        snprintf(last_slash + 1, sizeof(compat_sock) - (size_t)(last_slash - compat_sock) - 1, "%s", DOCKER_COMPAT_SOCK);
        unlink(compat_sock);
        symlink(cfg->socket_path, compat_sock);
    }

    printf("⚡ [nocker] Silicon Daemon Started Successfully\n");
    printf("   Socket : %s\n", cfg->socket_path);
    if (compat_sock[0]) {
        printf("   Compat : %s\n", compat_sock);
    }
    if (tcp_fd >= 0) {
        printf("   HTTP   : http://127.0.0.1:%d\n", cfg->port);
        printf("   Web UI : http://localhost:%d\n", cfg->port);
    }
    printf("   PID    : %d (Press Ctrl+C to terminate)\n", getpid());
    fflush(stdout);

    /* Event Loop */
    while (g_running) {
        struct pollfd pfds[2];
        int nfds = 0;

        pfds[0].fd = unix_fd;
        pfds[0].events = POLLIN;
        nfds++;

        if (tcp_fd >= 0) {
            pfds[1].fd = tcp_fd;
            pfds[1].events = POLLIN;
            nfds++;
        }

        int ret = poll(pfds, (nfds_t)nfds, 200); /* 200ms tick */
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (ret == 0) continue;

        /* Accept UNIX Socket Connection */
        if (pfds[0].revents & POLLIN) {
            int client_fd = accept(unix_fd, NULL, NULL);
            if (client_fd >= 0) {
                handle_client(client_fd);
            }
        }

        /* Accept TCP Socket Connection */
        if (tcp_fd >= 0 && (pfds[1].revents & POLLIN)) {
            int client_fd = accept(tcp_fd, NULL, NULL);
            if (client_fd >= 0) {
                handle_client(client_fd);
            }
        }
    }

    printf("\n⚡ [nocker] Shutting down daemon gracefully...\n");
    close(unix_fd);
    unlink(cfg->socket_path);
    if (compat_sock[0]) unlink(compat_sock);
    if (tcp_fd >= 0) {
        close(tcp_fd);
    }

    return 0;
}
