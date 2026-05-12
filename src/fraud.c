#include "fraud.h"
#include "dataset.h"
#include <math.h>
#include <stdio.h>
#include <float.h>

/* ──────────────────────────────────────────────
 * Cosine similarity between a float query vector
 * and a quantized uint8 reference vector.
 *
 * The query vector may contain -1 sentinels for
 * dimensions with missing data — those dimensions
 * are skipped entirely in the computation.
 *
 * The reference vector is valid quantized data
 * (never has -1 sentinels).
 * ────────────────────────────────────────────── */

static float cosine_similarity_quantized(const float *query,
                                          const uint8_t *ref,
                                          const struct quant_params *params) {
    float dot = 0.0f, nq = 0.0f, nr = 0.0f;

    for (int i = 0; i < VECTOR_LEN; i++) {
        /* Skip dimensions with missing data in the query */
        if (query[i] == -1.0f)
            continue;

        /* Decode reference dimension on the fly */
        float rv = qdecode(ref[i], params->mins[i], params->ranges[i]);

        dot += query[i] * rv;
        nq += query[i] * query[i];
        nr += rv * rv;
    }

    if (nq == 0.0f || nr == 0.0f)
        return 0.0f;

    return dot / (sqrtf(nq) * sqrtf(nr));
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

    /* Scan all entries */
    for (size_t i = 0; i < g_dataset.count; i++) {
        float sim = cosine_similarity_quantized(vec,
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
