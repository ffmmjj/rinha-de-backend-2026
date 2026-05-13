/* h2o must use the internal evloop (not libuv) to match server.c */
#define H2O_USE_LIBUV 0

#include "routes.h"
#include "transaction.h"
#include "fraud.h"
#include "features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <h2o.h>

/* ──────────────────────────────────────────────
 * Global dataset (defined in server.c)
 * ────────────────────────────────────────────── */

extern struct dataset g_dataset;

/* ──────────────────────────────────────────────
 * Reusable helper: send a JSON response
 *
 * Note: body must live at least until the response
 * is sent (we use h2o_send_inline which copies it).
 * ────────────────────────────────────────────── */

static void respond_json(h2o_req_t *req, int status, const char *body) {
    req->res.status = status;
    req->res.reason = status == 200 ? "OK"
                     : status == 400 ? "Bad Request"
                     : status == 404 ? "Not Found"
                     : "Internal Server Error";
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE,
                   NULL, H2O_STRLIT("application/json"));
    h2o_send_inline(req, body, strlen(body));
}

/* ──────────────────────────────────────────────
 * GET /ready
 * ────────────────────────────────────────────── */

int handle_ready(h2o_handler_t *self, h2o_req_t *req) {
    (void)self;
    if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
        return -1;

    respond_json(req, 200, "{\"status\":\"ok\"}");
    return 0;
}

/* ──────────────────────────────────────────────
 * POST /fraud-score
 *
 * The POST body is in req->entity (h2o already
 * buffered it for us via max_request_entity_size).
 * ────────────────────────────────────────────── */

int handle_fraud_score(h2o_handler_t *self, h2o_req_t *req) {
    (void)self;
    if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("POST")))
        return -1;

    if (req->entity.base == NULL || req->entity.len == 0) {
        respond_json(req, 400, "{\"error\":\"empty_body\"}");
        return 0;
    }

    struct transaction tx;
    if (transaction_parse(req->entity.base, req->entity.len, &tx) != 0) {
        respond_json(req, 400, "{\"error\":\"invalid_json\"}");
        return 0;
    }

    fprintf(stderr, "--- Transaction ---\n");
    fprintf(stderr, "  id: %s\n", tx.id);
    fprintf(stderr, "  amount: %.2f\n", tx.transaction.amount);
    fprintf(stderr, "  installments: %d\n", tx.transaction.installments);
    fprintf(stderr, "  requested_at: %s\n", tx.transaction.requested_at);
    fprintf(stderr, "  customer avg_amount: %.2f\n", tx.customer.avg_amount);
    fprintf(stderr, "  customer tx_count_24h: %d\n", tx.customer.tx_count_24h);
    fprintf(stderr, "  customer known_merchants [%zu]:",
            tx.customer.known_merchants_count);
    for (size_t i = 0; i < tx.customer.known_merchants_count; i++) {
        fprintf(stderr, " %s", tx.customer.known_merchants[i]);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "  merchant id: %s\n", tx.merchant.id);
    fprintf(stderr, "  merchant mcc: %s\n", tx.merchant.mcc);
    fprintf(stderr, "  merchant avg_amount: %.2f\n", tx.merchant.avg_amount);
    fprintf(stderr, "  terminal is_online: %d\n", tx.terminal.is_online);
    fprintf(stderr, "  terminal card_present: %d\n", tx.terminal.card_present);
    fprintf(stderr, "  terminal km_from_home: %.1f\n", tx.terminal.km_from_home);

    if (tx.last) {
        fprintf(stderr, "  last_transaction:\n");
        fprintf(stderr, "    timestamp: %s\n", tx.last->timestamp);
        fprintf(stderr, "    km_from_current: %.1f\n", tx.last->km_from_current);
    } else {
        fprintf(stderr, "  last_transaction: null\n");
    }

    float vec[VECTOR_LEN];
    features_extract(&tx, vec);

    fprintf(stderr, "  feature_vector:");
    for (size_t i = 0; i < VECTOR_LEN; i++) {
        fprintf(stderr, " %.4f", vec[i]);
    }
    fprintf(stderr, "\n");

    bool is_fraud = fraud_detect(vec);

    fprintf(stderr, "  fraud: %s\n", is_fraud ? "YES" : "NO");

    transaction_free(&tx);

    if (is_fraud) {
        respond_json(req, 200, "{\"approved\":false,\"fraud_score\":1.0}");
    } else {
        respond_json(req, 200, "{\"approved\":true,\"fraud_score\":0.0}");
    }

    return 0;
}

/* ──────────────────────────────────────────────
 * Catch-all: 404
 * ────────────────────────────────────────────── */

int handle_not_found(h2o_handler_t *self, h2o_req_t *req) {
    (void)self;
    respond_json(req, 404, "{\"error\":\"not_found\"}");
    return 0;
}
