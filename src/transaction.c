#include "transaction.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int transaction_parse(const char *json, size_t len,
                       struct transaction *tx) {
    memset(tx, 0, sizeof(*tx));

    /* Top-level fields */
    char *s;

    s = json_get_string(json, len, "id");
    if (s) { strncpy(tx->id, s, sizeof(tx->id) - 1); free(s); }

    /* transaction.* */
    tx->transaction.amount = json_get_number(json, len, "transaction.amount");
    tx->transaction.installments = json_get_int(json, len, "transaction.installments");

    s = json_get_string(json, len, "transaction.requested_at");
    if (s) { strncpy(tx->transaction.requested_at, s,
                      sizeof(tx->transaction.requested_at) - 1); free(s); }

    /* customer.* */
    tx->customer.avg_amount = json_get_number(json, len, "customer.avg_amount");
    tx->customer.tx_count_24h = json_get_int(json, len, "customer.tx_count_24h");

    tx->customer.known_merchants_count =
        json_get_string_array(json, len, "customer.known_merchants",
                               tx->customer.known_merchants, MAX_MERCHANTS);

    /* merchant.* */
    s = json_get_string(json, len, "merchant.id");
    if (s) { strncpy(tx->merchant.id, s, sizeof(tx->merchant.id) - 1); free(s); }

    s = json_get_string(json, len, "merchant.mcc");
    if (s) { strncpy(tx->merchant.mcc, s, sizeof(tx->merchant.mcc) - 1); free(s); }

    tx->merchant.avg_amount = json_get_number(json, len, "merchant.avg_amount");

    /* terminal.* */
    tx->terminal.is_online = json_get_bool(json, len, "terminal.is_online");
    tx->terminal.card_present = json_get_bool(json, len, "terminal.card_present");
    tx->terminal.km_from_home = json_get_number(json, len, "terminal.km_from_home");

    /* last_transaction (nullable) */
    tx->last = NULL;

    /* Check if last_transaction exists by looking for "last_transaction.timestamp" */
    s = json_get_string(json, len, "last_transaction.timestamp");
    if (s) {
        tx->last = (struct last_transaction_info *)malloc(sizeof(*tx->last));
        if (tx->last) {
            memset(tx->last, 0, sizeof(*tx->last));
            strncpy(tx->last->timestamp, s, sizeof(tx->last->timestamp) - 1);
            tx->last->km_from_current =
                json_get_number(json, len, "last_transaction.km_from_current");
        }
        free(s);
    }

    return 0;
}

void transaction_free(struct transaction *tx) {
    if (tx == NULL) return;
    for (size_t i = 0; i < tx->customer.known_merchants_count; i++) {
        free(tx->customer.known_merchants[i]);
    }
    free(tx->last);
}
