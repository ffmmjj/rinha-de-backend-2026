#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <stdbool.h>
#include <stddef.h>

/* ──────────────────────────────────────────────
 * Nested structures matching the JSON body
 * ────────────────────────────────────────────── */

#define MAX_MERCHANTS 32

struct transaction_amounts {
    double amount;
    int installments;
    char requested_at[64];  /* ISO 8601 string */
};

struct customer_info {
    double avg_amount;
    int tx_count_24h;
    char *known_merchants[MAX_MERCHANTS];
    size_t known_merchants_count;
};

struct merchant_info {
    char id[128];
    char mcc[16];
    double avg_amount;
};

struct terminal_info {
    bool is_online;
    bool card_present;
    double km_from_home;
};

struct last_transaction_info {
    char timestamp[64];
    double km_from_current;
};

struct transaction {
    char id[128];
    struct transaction_amounts transaction;
    struct customer_info customer;
    struct merchant_info merchant;
    struct terminal_info terminal;
    struct last_transaction_info *last;  /* NULL if absent */
};

/* ──────────────────────────────────────────────
 * Parse JSON body into a transaction struct.
 * Returns 0 on success, -1 on parse error.
 * The caller must free string members via transaction_free().
 * ────────────────────────────────────────────── */

int transaction_parse(const char *json, size_t len,
                       struct transaction *tx);

void transaction_free(struct transaction *tx);

#endif /* TRANSACTION_H */
