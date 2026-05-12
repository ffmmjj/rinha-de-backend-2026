#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define VECTOR_LEN 14

/* ──────────────────────────────────────────────
 * Per-dimension quantization parameters
 * ────────────────────────────────────────────── */

struct quant_params {
    float mins[VECTOR_LEN];
    float ranges[VECTOR_LEN];
};

/* ──────────────────────────────────────────────
 * A single reference entry — 8-bit quantized.
 * Layout matches resources/references.bin:
 *   [0..13]  uint8_t[14]  quantized vector
 *   [14]     uint8_t      label (0 = legit, 1 = fraud)
 * Total: 15 bytes per entry.
 * ────────────────────────────────────────────── */

struct reference {
    uint8_t qvector[VECTOR_LEN];
    uint8_t label;
};

_Static_assert(sizeof(struct reference) == 15, "reference struct size mismatch");

/* ──────────────────────────────────────────────
 * Decode a quantized value back to float.
 * ────────────────────────────────────────────── */

static inline float qdecode(uint8_t q, float dim_min, float dim_range) {
    return (float)q / 255.0f * dim_range + dim_min;
}

/* ──────────────────────────────────────────────
 * Dataset loaded into memory from a binary file.
 *
 * Ownership semantics:
 *   dataset_load() mmaps the file.
 *   dataset_free() unmaps it.
 *   The `entries` pointer points into the mmap'd region.
 * ────────────────────────────────────────────── */

struct dataset {
    struct quant_params params;
    struct reference *entries;
    size_t count;
    void *_mem;      /* mmap base */
    size_t _mem_len; /* size of mapped region */
};

/* Quantize a float vector using the dataset's params.
   Dimensions with -1 are left as 0 (skipped in similarity). */
static inline void quantize_query(const struct dataset *ds,
                                   const float *vec,
                                   uint8_t *qvec) {
    for (size_t i = 0; i < VECTOR_LEN; i++) {
        if (vec[i] == -1.0f) {
            qvec[i] = 0;
        } else {
            float clamped = vec[i];
            if (clamped < 0.0f) clamped = 0.0f;
            if (clamped > 1.0f) clamped = 1.0f;
            qvec[i] = (uint8_t)(clamped / ds->params.ranges[i] * 255.0f
                                + 0.5f);
        }
    }
}

/* Load references.bin into memory. Returns 0 on success. */
int dataset_load(const char *path, struct dataset *ds);

/* Unload / free the dataset. */
void dataset_free(struct dataset *ds);

/* Convenience: return pointer to i-th entry (bounds-checked). */
static inline struct reference *dataset_get(const struct dataset *ds, size_t i) {
    if (i >= ds->count) return NULL;
    return &ds->entries[i];
}

/* Decode a full entry into a float vector (convenience). */
static inline void reference_decode(const struct dataset *ds,
                                     const struct reference *ref,
                                     float *out) {
    for (size_t i = 0; i < VECTOR_LEN; i++) {
        out[i] = qdecode(ref->qvector[i], ds->params.mins[i], ds->params.ranges[i]);
    }
}

#endif /* DATASET_H */
