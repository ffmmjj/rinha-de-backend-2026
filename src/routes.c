#include "routes.h"
#include "transaction.h"
#include "fraud.h"
#include "features.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ──────────────────────────────────────────────
 * Reusable helper: queue a JSON response
 * ────────────────────────────────────────────── */

static enum MHD_Result respond_json(struct MHD_Connection *connection,
                                     unsigned int status,
                                     const char *body) {
    struct MHD_Response *response =
        MHD_create_response_from_buffer(strlen(body), (void *)body,
                                         MHD_RESPMEM_PERSISTENT);
    if (response == NULL) return MHD_NO;

    MHD_add_response_header(response, "Content-Type", "application/json");
    const enum MHD_Result ret =
        MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return ret;
}

/* ──────────────────────────────────────────────
 * Route handlers
 * ────────────────────────────────────────────── */

enum MHD_Result handle_ready(struct MHD_Connection *connection) {
    return respond_json(connection, MHD_HTTP_OK, "{\"status\":\"ok\"}");
}

enum MHD_Result handle_fraud_score(struct MHD_Connection *connection,
                                    const struct request_body *body) {
    struct transaction tx;

    if (transaction_parse(body->data, body->len, &tx) != 0) {
        return respond_json(connection, MHD_HTTP_BAD_REQUEST,
                             "{\"error\":\"invalid_json\"}");
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

    bool is_fraud = fraud_detect(&tx);

    fprintf(stderr, "  fraud: %s\n", is_fraud ? "YES" : "NO");

    transaction_free(&tx);

    if (is_fraud) {
        return respond_json(connection, MHD_HTTP_OK,
                             "{\"approved\":false,\"fraud_score\":1.0}");
    } else {
        return respond_json(connection, MHD_HTTP_OK,
                             "{\"approved\":true,\"fraud_score\":0.0}");
    }
}

enum MHD_Result handle_not_found(struct MHD_Connection *connection) {
    return respond_json(connection, MHD_HTTP_NOT_FOUND,
                         "{\"error\":\"not_found\"}");
}
