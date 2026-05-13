/* ── Force h2o to use its internal evloop (not libuv) ── */
#define H2O_USE_LIBUV 0

#include "server_h2o.h"
#include "routes_h2o.h"
#include "dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <h2o.h>
#include <h2o/http1.h>

/* ──────────────────────────────────────────────
 * Global dataset definition
 * ────────────────────────────────────────────── */

struct dataset g_dataset;

/* ──────────────────────────────────────────────
 * Global h2o objects (needed by accept callback)
 * ────────────────────────────────────────────── */

static h2o_globalconf_t config;
static h2o_context_t ctx;
static h2o_accept_ctx_t accept_ctx;

/* ──────────────────────────────────────────────
 * Accept callback — called when a new connection arrives
 * ────────────────────────────────────────────── */

static void on_accept(h2o_socket_t *listener, const char *err) {
    h2o_socket_t *sock;

    if (err != NULL) {
        return;
    }

    if ((sock = h2o_evloop_socket_accept(listener)) == NULL)
        return;
    h2o_accept(&accept_ctx, sock);
}

/* ──────────────────────────────────────────────
 * Create the listening socket & register with the event loop
 * ────────────────────────────────────────────── */

static int create_listener(unsigned short port) {
    struct sockaddr_in addr;
    int fd, reuseaddr_flag = 1;
    h2o_socket_t *sock;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_flag,
                   sizeof(reuseaddr_flag)) != 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, SOMAXCONN) != 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    sock = h2o_evloop_socket_create(ctx.loop, fd, H2O_SOCKET_FLAG_DONT_READ);
    h2o_socket_read_start(sock, on_accept);

    return 0;
}

/* ──────────────────────────────────────────────
 * Helper: register a handler for a given path
 * ────────────────────────────────────────────── */

static h2o_pathconf_t *register_handler(h2o_hostconf_t *hostconf,
                                        const char *path,
                                        int (*on_req)(h2o_handler_t *,
                                                       h2o_req_t *)) {
    h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, path, 0);
    h2o_handler_t *handler = h2o_create_handler(pathconf, sizeof(*handler));
    handler->on_req = on_req;
    return pathconf;
}

/* ──────────────────────────────────────────────
 * Start the server — never returns
 * ────────────────────────────────────────────── */

int server_h2o_start(unsigned short port) {
    h2o_hostconf_t *hostconf;

    h2o_config_init(&config);
    hostconf = h2o_config_register_host(
        &config, h2o_iovec_init(H2O_STRLIT("default")), 65535);

    /* Register route handlers */
    register_handler(hostconf, "/ready", handle_ready_h2o);
    register_handler(hostconf, "/fraud-score", handle_fraud_score_h2o);

    /* Default handler for everything else (404) */
    register_handler(hostconf, "/", handle_not_found_h2o);

    /* Set max request entity size (allow reasonably large POST bodies) */
    config.max_request_entity_size = 1024 * 1024;  /* 1 MB */

    /* Initialize the h2o context with the evloop */
    h2o_context_init(&ctx, h2o_evloop_create(), &config);

    accept_ctx.ctx = &ctx;
    accept_ctx.hosts = config.hosts;

    if (create_listener(port) != 0) {
        fprintf(stderr, "failed to listen on 0.0.0.0:%u: %s\n",
                port, strerror(errno));
        return -1;
    }

    fprintf(stderr, "Listening on http://0.0.0.0:%u (h2o/evloop)\n", port);
    fflush(stderr);

    /* Run event loop until killed */
    while (h2o_evloop_run(ctx.loop, INT32_MAX) == 0)
        ;

    return 0;
}
