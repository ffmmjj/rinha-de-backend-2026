#ifndef ROUTES_H
#define ROUTES_H

#include <microhttpd.h>

/* ──────────────────────────────────────────────
 * Per-connection state for accumulating POST bodies
 * ────────────────────────────────────────────── */

struct request_body {
    char *data;
    size_t len;
    size_t cap;
};

/* ──────────────────────────────────────────────
 * Route handlers
 *
 * Each returns MHD_YES on success, MHD_NO on error.
 * ────────────────────────────────────────────── */

enum MHD_Result handle_ready(struct MHD_Connection *connection);

enum MHD_Result handle_fraud_score(struct MHD_Connection *connection,
                                    const struct request_body *body);

enum MHD_Result handle_not_found(struct MHD_Connection *connection);

#endif /* ROUTES_H */
