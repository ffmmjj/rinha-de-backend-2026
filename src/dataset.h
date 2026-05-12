#ifndef DATASET_H
#define DATASET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define VECTOR_LEN 14

/* ──────────────────────────────────────────────
 * A single reference entry — layout matches
 * resources/references.bin exactly.
 * ────────────────────────────────────────────── */

struct reference {
    float vector[VECTOR_LEN];
    uint8_t label; /* 0 = legit, 1 = fraud */
    uint8_t _pad[3]; /* ensure struct is 60 bytes, no padding surprises */
};

_Static_assert(sizeof(struct reference) == 60, "reference struct size mismatch");

/* ──────────────────────────────────────────────
 * Dataset loaded into memory from a binary file.
 *
 * Ownership semantics:
 *   dataset_load() mmaps (or allocates) the data.
 *   dataset_free() unmaps / frees it.
 *   The `entries` pointer points into that memory.
 * ────────────────────────────────────────────── */

struct dataset {
    struct reference *entries;
    size_t count;
    void *_mem;      /* opaque: mmap base or malloc pointer */
    size_t _mem_len; /* size of mapped / allocated region */
};

/* Load references.bin into memory. Returns 0 on success. */
int dataset_load(const char *path, struct dataset *ds);

/* Unload / free the dataset. */
void dataset_free(struct dataset *ds);

/* Convenience: return pointer to i-th entry (bounds-checked). */
static inline struct reference *dataset_get(const struct dataset *ds, size_t i) {
    if (i >= ds->count) return NULL;
    return &ds->entries[i];
}

#endif /* DATASET_H */
