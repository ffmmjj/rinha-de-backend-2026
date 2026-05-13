#include "fraud.h"
#include "dataset.h"
#include <math.h>
#include <stdio.h>
#include <float.h>

/* ──────────────────────────────────────────────
 * Dot product between a float query vector
 * and a quantized uint8 reference vector.
 *
 * The query vector may contain -1 sentinels for
 * dimensions with missing data — those dimensions
 * are skipped entirely in the computation.
 *
 * The reference vector is valid quantized data
 * (never has -1 sentinels).
 *
 * The query norm is passed in to avoid recomputing
 * it for every comparison.
 * ────────────────────────────────────────────── */

static float cosine_similarity_quantized(const float *query, float query_norm,
                                          const uint8_t *ref,
                                          const struct quant_params *params) {
    float dot = 0.0f, nr = 0.0f;

    for (int i = 0; i < VECTOR_LEN; i++) {
        /* Skip dimensions with missing data in the query */
        if (query[i] == -1.0f)
            continue;

        /* Decode reference dimension on the fly */
        float rv = qdecode(ref[i], params->mins[i], params->ranges[i]);

        dot += query[i] * rv;
        nr += rv * rv;
    }

    if (query_norm == 0.0f || nr == 0.0f)
        return 0.0f;

    return dot / (query_norm * sqrtf(nr));
}

/* ──────────────────────────────────────────────
 * Fraud detection via 5-NN cosine similarity
 * ────────────────────────────────────────────── */

bool fraud_detect(const float vec[14]) {
    extern struct dataset g_dataset;

    if (g_dataset.count == 0) {
        fprintf(stderr, "fraud_detect: dataset is empty\n");
        return false;
    }

    /*
     * Keep a small array of the 5 best (similarity, index) pairs.
     * Initially filled with -inf similarity.
     */
    float best_sim[KNN_K];
    size_t best_idx[KNN_K];

    for (int i = 0; i < KNN_K; i++) {
        best_sim[i] = -FLT_MAX;
        best_idx[i] = 0;
    }

    /* Compute query norm once (dimensions with -1.0 are skipped) */
    float query_norm = 0.0f;
    for (int i = 0; i < VECTOR_LEN; i++) {
        if (vec[i] == -1.0f)
            continue;
        query_norm += vec[i] * vec[i];
    }
    query_norm = query_norm > 0.0f ? sqrtf(query_norm) : 1.0f;

    /* Scan all entries */
    for (size_t i = 0; i < g_dataset.count; i++) {
        float sim = cosine_similarity_quantized(vec, query_norm,
                                                 g_dataset.entries[i].qvector,
                                                 &g_dataset.params);

        /* Insert into top-5 if better than the worst so far */
        if (sim > best_sim[KNN_K - 1]) {
            int pos = KNN_K - 1;
            while (pos > 0 && sim > best_sim[pos - 1]) {
                best_sim[pos] = best_sim[pos - 1];
                best_idx[pos] = best_idx[pos - 1];
                pos--;
            }
            best_sim[pos] = sim;
            best_idx[pos] = i;
        }
    }

    /* Count how many of the 5 nearest are fraud */
    int fraud_count = 0;
    for (int i = 0; i < KNN_K; i++) {
        if (g_dataset.entries[best_idx[i]].label == 1)
            fraud_count++;
    }

    fprintf(stderr, "  5-NN (cosine):");
    for (int i = 0; i < KNN_K; i++) {
        fprintf(stderr, " [idx=%zu sim=%.4f label=%s]",
                best_idx[i], best_sim[i],
                g_dataset.entries[best_idx[i]].label ? "fraud" : "legit");
    }
    fprintf(stderr, " fraud_count=%d threshold=%d\n", fraud_count, KNN_FRAUD_THRESHOLD);

    return fraud_count >= KNN_FRAUD_THRESHOLD;
}
