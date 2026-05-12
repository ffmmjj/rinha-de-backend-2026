#include "server.h"
#include "routes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
 * Global dataset definition
 * ────────────────────────────────────────────── */

struct dataset g_dataset;

/* ──────────────────────────────────────────────
 * Iteration tracking for POST body accumulation
 * ────────────────────────────────────────────── */

static void accumulate_body(struct request_body *body,
                             const char *data, size_t len) {
    if (len == 0) return;
    size_t needed = body->len + len + 1;
    if (needed > body->cap) {
        body->cap = needed + 4096;
        char *p = (char *)realloc(body->data, body->cap);
        if (p == NULL) return;
        body->data = p;
    }
    memcpy(body->data + body->len, data, len);
    body->len += len;
    body->data[body->len] = '\0';
}

/* ──────────────────────────────────────────────
 * Request dispatcher (called by MHD thread pool)
 *
 * MHD calls this handler multiple times per connection:
 *   1. First call (*con_cls == NULL): allocate state, return MHD_YES.
 *      Do NOT queue a response yet.
 *   2. Intermediate calls (*upload_data_size > 0): accumulate body.
 *   3. Final call (*upload_data_size == 0, *con_cls != NULL):
 *      dispatch route and queue response.
 * ────────────────────────────────────────────── */

static enum MHD_Result request_handler(void *cls,
                                        struct MHD_Connection *connection,
                                        const char *url,
                                        const char *method,
                                        const char *version,
                                        const char *upload_data,
                                        size_t *upload_data_size,
                                        void **con_cls) {
    (void)cls;
    (void)version;

    struct request_body *body = *con_cls;

    if (body == NULL) {
        /* First call: allocate state and wait for body data */
        body = (struct request_body *)calloc(1, sizeof(*body));
        if (body == NULL) return MHD_NO;
        *con_cls = body;
        return MHD_YES;
    }

    if (*upload_data_size > 0) {
        accumulate_body(body, upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    /* ── *upload_data_size == 0: body fully received, dispatch ── */

    if (strcmp(method, "GET") == 0 && strcmp(url, "/ready") == 0) {
        enum MHD_Result ret = handle_ready(connection);
        free(body->data);
        free(body);
        *con_cls = NULL;
        return ret;
    }

    if (strcmp(method, "POST") == 0 && strcmp(url, "/fraud-score") == 0) {
        enum MHD_Result ret = handle_fraud_score(connection, body);
        free(body->data);
        free(body);
        *con_cls = NULL;
        return ret;
    }

    {
        enum MHD_Result ret = handle_not_found(connection);
        free(body->data);
        free(body);
        *con_cls = NULL;
        return ret;
    }
}

/* ──────────────────────────────────────────────
 * Server lifecycle
 * ────────────────────────────────────────────── */

struct MHD_Daemon *server_start(unsigned short port, int worker_count) {
    return MHD_start_daemon(
        MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD,
        port,
        NULL, NULL,
        &request_handler, NULL,
        MHD_OPTION_THREAD_POOL_SIZE, worker_count,
        MHD_OPTION_CONNECTION_LIMIT, 10000,
        MHD_OPTION_CONNECTION_TIMEOUT, 30,
        MHD_OPTION_END);
}

void server_stop(struct MHD_Daemon *daemon) {
    if (daemon != NULL) {
        MHD_stop_daemon(daemon);
    }
}
