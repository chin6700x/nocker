/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * ⚡ nocker: Not Docker · Native Darwin Seatbelt & Watchdog Process Supervisor
 */

#include "native_seatbelt.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <poll.h>

#if defined(__APPLE__)
extern int sandbox_init(const char *profile, uint64_t flags, char **errorbuf);
extern void sandbox_free_error(char *errorbuf);
#endif

int native_seatbelt_run(const seatbelt_spec_t *spec, seatbelt_result_t *res) {
    if (!spec || !res || !spec->command[0]) return -1;
    memset(res, 0, sizeof(seatbelt_result_t));

    int timeout_ms = spec->timeout_ms > 0 ? spec->timeout_ms : 5000;
    int max_mem_mb = spec->max_mem_mb > 0 ? spec->max_mem_mb : 512;

    int out_pipe[2];
    int err_pipe[2];
    if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
        snprintf(res->error_msg, sizeof(res->error_msg), "Failed to allocate IPC pipes");
        return -1;
    }

    double t0 = engine_get_time_ms();
    pid_t pid = fork();

    if (pid < 0) {
        snprintf(res->error_msg, sizeof(res->error_msg), "Fork failed");
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return -1;
    }

    if (pid == 0) {
        /* Child Process */
        close(out_pipe[0]);
        close(err_pipe[0]);

        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);
        close(out_pipe[1]);
        close(err_pipe[1]);

        /* Redirect STDIN to /dev/null */
        int dev_null = open("/dev/null", O_RDONLY);
        if (dev_null >= 0) {
            dup2(dev_null, STDIN_FILENO);
            close(dev_null);
        }

        /* Working directory */
        if (spec->workdir[0]) {
            struct stat st;
            if (stat(spec->workdir, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (chdir(spec->workdir) != 0) {
                    /* ignore */
                }
            }
        }

        /* Export environment variables */
        for (int i = 0; i < spec->env_count; i++) {
            if (spec->env_keys[i][0]) {
                setenv(spec->env_keys[i], spec->env_vals[i], 1);
            }
        }
        setenv("SANDBOX_ENGINE", "1", 1);
        setenv("SILICON_VELOCITY", "baremetal", 1);

        /* Resource limits */
        struct rlimit rl_mem;
        rl_mem.rlim_cur = (rlim_t)max_mem_mb * 1024 * 1024;
        rl_mem.rlim_max = (rlim_t)max_mem_mb * 1024 * 1024;
        setrlimit(RLIMIT_DATA, &rl_mem);

        struct rlimit rl_cpu;
        rlim_t cpu_sec = (timeout_ms / 1000) + 1;
        rl_cpu.rlim_cur = cpu_sec;
        rl_cpu.rlim_max = cpu_sec + 2;
        setrlimit(RLIMIT_CPU, &rl_cpu);

#if defined(__APPLE__)
        if (spec->strict_sandbox) {
            char *sberr = NULL;
            const char *profile =
                "(version 1)\n"
                "(deny default)\n"
                "(allow process-exec)\n"
                "(allow process-fork)\n"
                "(allow sysctl-read)\n"
                "(allow file-read*)\n"
                "(allow file-write* (subpath \"/tmp\"))\n"
                "(allow file-write* (subpath \"/private/tmp\"))\n"
                "(allow file-write* (subpath \"/var/folders\"))\n"
                "(allow file-write* (subpath \"/private/var/folders\"))\n"
                "(allow file-write* (literal \"/dev/null\"))\n"
                "(allow file-write* (literal \"/dev/zero\"))\n"
                "(allow file-write* (literal \"/dev/dtracehelper\"))\n"
                "(allow file-write* (subpath \"/Volumes/ZeroRAM\"))\n";
            sandbox_init(profile, 0, &sberr);
            if (sberr) sandbox_free_error(sberr);
        }
#endif

        execl("/bin/sh", "/bin/sh", "-c", spec->command, (char *)NULL);
        _exit(127);
    }

    /* Parent Process - High-Precision Watchdog Monitor */
    close(out_pipe[1]);
    close(err_pipe[1]);

    fcntl(out_pipe[0], F_SETFL, O_NONBLOCK);
    fcntl(err_pipe[0], F_SETFL, O_NONBLOCK);

    size_t out_idx = 0;
    size_t err_idx = 0;
    bool child_exited = false;
    int status = 0;

    int sleep_chunk_us = 5000; /* 5ms polling tick */
    int elapsed_us = 0;
    int timeout_us = timeout_ms * 1000;

    while (elapsed_us < timeout_us) {
        pid_t wp = waitpid(pid, &status, WNOHANG);
        if (wp == pid) {
            child_exited = true;
            break;
        }

        /* Drain pipes */
        char buf[1024];
        ssize_t n;
        while ((n = read(out_pipe[0], buf, sizeof(buf))) > 0) {
            if (out_idx + (size_t)n < sizeof(res->stdout_buf) - 1) {
                memcpy(res->stdout_buf + out_idx, buf, (size_t)n);
                out_idx += (size_t)n;
            }
        }
        while ((n = read(err_pipe[0], buf, sizeof(buf))) > 0) {
            if (err_idx + (size_t)n < sizeof(res->stderr_buf) - 1) {
                memcpy(res->stderr_buf + err_idx, buf, (size_t)n);
                err_idx += (size_t)n;
            }
        }

        usleep((useconds_t)sleep_chunk_us);
        elapsed_us += sleep_chunk_us;
    }

    if (!child_exited) {
        /* Watchdog Timeout Triggered */
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        res->timed_out = true;
        res->exit_code = 124;
    } else {
        if (WIFEXITED(status)) {
            res->exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            res->term_signal = WTERMSIG(status);
            res->exit_code = 128 + res->term_signal;
            if (res->term_signal == SIGXCPU) {
                res->timed_out = true;
            }
        }
    }

    /* Drain remaining output */
    char buf[1024];
    ssize_t n;
    while ((n = read(out_pipe[0], buf, sizeof(buf))) > 0) {
        if (out_idx + (size_t)n < sizeof(res->stdout_buf) - 1) {
            memcpy(res->stdout_buf + out_idx, buf, (size_t)n);
            out_idx += (size_t)n;
        }
    }
    while ((n = read(err_pipe[0], buf, sizeof(buf))) > 0) {
        if (err_idx + (size_t)n < sizeof(res->stderr_buf) - 1) {
            memcpy(res->stderr_buf + err_idx, buf, (size_t)n);
            err_idx += (size_t)n;
        }
    }
    res->stdout_buf[out_idx] = '\0';
    res->stderr_buf[err_idx] = '\0';

    close(out_pipe[0]);
    close(err_pipe[0]);

    double t1 = engine_get_time_ms();
    res->exec_time_ms = t1 - t0;
    return 0;
}
