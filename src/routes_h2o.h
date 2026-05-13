#ifndef ROUTES_H2O_H
#define ROUTES_H2O_H

/* NOTE: h2o.h is NOT included here because server_h2o.c and routes_h2o.c
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

int handle_ready_h2o(h2o_handler_t *self, h2o_req_t *req);

int handle_fraud_score_h2o(h2o_handler_t *self, h2o_req_t *req);

int handle_not_found_h2o(h2o_handler_t *self, h2o_req_t *req);

#endif /* ROUTES_H2O_H */
