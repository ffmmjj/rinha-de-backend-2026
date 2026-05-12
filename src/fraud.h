#ifndef FRAUD_H
#define FRAUD_H

#include "transaction.h"
#include <stdbool.h>

/* ──────────────────────────────────────────────
 * Fraud detection
 *
 * Analyses a transaction and returns true if it
 * is suspected to be fraudulent, false otherwise.
 * ────────────────────────────────────────────── */

bool fraud_detect(const struct transaction *tx);

#endif /* FRAUD_H */
