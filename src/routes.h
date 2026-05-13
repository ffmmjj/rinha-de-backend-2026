#ifndef ROUTES_H
#define ROUTES_H

/* NOTE: h2o.h is NOT included here because server.c and routes.c
 * both must #define H2O_USE_LIBUV 0 before including it. */

/* Forward declarations for h2o types (incomplete) */
typedef struct st_h2o_handler_t h2o_handler_t;
typedef struct st_h2o_req_t h2o_req_t;

/* ──────────────────────────────────────────────
 * h2o route handlers
 *
 * Each returns 0 on success, -1 if the request
 * should be passed to the next handler.
 * ────────────────────────────────────────────── */

int handle_ready(h2o_handler_t *self, h2o_req_t *req);

int handle_fraud_score(h2o_handler_t *self, h2o_req_t *req);

int handle_not_found(h2o_handler_t *self, h2o_req_t *req);

#endif /* ROUTES_H */
