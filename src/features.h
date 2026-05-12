#ifndef FEATURES_H
#define FEATURES_H

#include "transaction.h"

#define VECTOR_LEN 14

/* ──────────────────────────────────────────────
 * Normalization constants
 * ────────────────────────────────────────────── */

struct norm_constants {
    double max_amount;
    double max_installments;
    double amount_vs_avg_ratio;
    double max_minutes;
    double max_km;
    double max_tx_count_24h;
    double max_merchant_avg_amount;
};

extern const struct norm_constants g_norm;

/* ──────────────────────────────────────────────
 * Transform a transaction into the 14D feature vector.
 * Out vector values are in [0, 1], except for indices
 * 5 and 6 which can be -1 when last_transaction is null.
 * ────────────────────────────────────────────── */

void features_extract(const struct transaction *tx, float out[VECTOR_LEN]);

#endif /* FEATURES_H */
