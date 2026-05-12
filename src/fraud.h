#ifndef FRAUD_H
#define FRAUD_H

#include <stdbool.h>
#include <stddef.h>

#define KNN_K 5
#define KNN_FRAUD_THRESHOLD 4  /* at least 4 out of 5 nearest must be fraud */

/* ──────────────────────────────────────────────
 * Fraud detection
 *
 * Given a 14D feature vector, finds the 5 closest
 * entries in the reference dataset by cosine similarity.
 * Returns true (fraud) if at least 4 of the 5 nearest
 * neighbors are labeled as fraud.
 * ────────────────────────────────────────────── */

bool fraud_detect(const float vec[14]);

#endif /* FRAUD_H */
