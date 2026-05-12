#include "fraud.h"
#include "dataset.h"
#include <math.h>
#include <stdio.h>
#include <float.h>

/* ──────────────────────────────────────────────
 * Cosine similarity between two raw (non-quantized)
 * float vectors. Output is in [-1, 1].
 *
 * For vectors with -1 sentinel values (missing data),
 * we handle by zeroing the contribution from both vectors
 * when either has -1 — this avoids penalizing missing data.
 * ────────────────────────────────────────────── */

static float cosine_similarity(const float *a, const float *b) {
    float dot = 0.0f, na = 0.0f, nb = 0.0f;

    for (int i = 0; i < VECTOR_LEN; i++) {
        /* Skip dimensions where either vector has -1 (missing data) */
        if (a[i] == -1.0f || b[i] == -1.0f)
            continue;

        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }

    if (na == 0.0f || nb == 0.0f)
        return 0.0f;

    return dot / (sqrtf(na) * sqrtf(nb));
}

/* ──────────────────────────────────────────────
 * Decode a quantized reference entry into a
 * float vector using the dataset's params.
 * ────────────────────────────────────────────── */

static void decode_reference(const struct dataset *ds, size_t idx, float *out) {
    const struct reference *ref = &ds->entries[idx];
    for (int i = 0; i < VECTOR_LEN; i++) {
        out[i] = qdecode(ref->qvector[i],
                          ds->params.mins[i],
                          ds->params.ranges[i]);
    }
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
     * We keep a small array of the 5 best (similarity, index) pairs.
     * Initially filled with -inf similarity.
     */
    float best_sim[KNN_K];
    size_t best_idx[KNN_K];

    for (int i = 0; i < KNN_K; i++) {
        best_sim[i] = -FLT_MAX;
        best_idx[i] = 0;
    }

    /* Decode buffer for each reference (reused) */
    float ref_vec[VECTOR_LEN];

    /* Scan all entries */
    for (size_t i = 0; i < g_dataset.count; i++) {
        decode_reference(&g_dataset, i, ref_vec);
        float sim = cosine_similarity(vec, ref_vec);

        /* Insert into top-5 if better than the worst so far */
        if (sim > best_sim[KNN_K - 1]) {
            /* Find insertion point */
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
