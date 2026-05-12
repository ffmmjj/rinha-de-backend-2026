#include "dataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

/* ──────────────────────────────────────────────
 * Binary file format (little-endian):
 *
 *   [0..7]       uint64_t     N                (entry count)
 *   [8..63]      float[14]    dim_mins         (per-dimension min values)
 *   [64..119]    float[14]    dim_ranges       (per-dimension max - min)
 *   [120..]      struct reference[N]
 *                   [0..13]   uint8_t[14]      quantized vector
 *                   [14]      uint8_t          label
 *
 * Each entry: 15 bytes.
 *────────────────────────────────────────────── */

#define HEADER_SIZE (8 + VECTOR_LEN * 4 + VECTOR_LEN * 4)  /* 120 */
#define ENTRY_SIZE  sizeof(struct reference)               /* 15 */

int dataset_load(const char *path, struct dataset *ds) {
    memset(ds, 0, sizeof(*ds));

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("dataset_load: open");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("dataset_load: fstat");
        close(fd);
        return -1;
    }

    size_t file_size = (size_t)st.st_size;
    if (file_size < HEADER_SIZE) {
        fprintf(stderr, "dataset_load: file too small (%zu bytes)\n", file_size);
        close(fd);
        return -1;
    }

    /* mmap the entire file */
    void *base = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (base == MAP_FAILED) {
        perror("dataset_load: mmap");
        return -1;
    }

    /* Parse header */
    uint64_t count;
    memcpy(&count, base, sizeof(count));

    memcpy(ds->params.mins, (char *)base + 8, sizeof(ds->params.mins));
    memcpy(ds->params.ranges, (char *)base + 8 + VECTOR_LEN * 4,
           sizeof(ds->params.ranges));

    size_t expected_size = HEADER_SIZE + (size_t)count * ENTRY_SIZE;
    if (file_size < expected_size) {
        fprintf(stderr, "dataset_load: file truncated (expected %zu, got %zu)\n",
                expected_size, file_size);
        munmap(base, file_size);
        return -1;
    }

    ds->entries = (struct reference *)((char *)base + HEADER_SIZE);
    ds->count = (size_t)count;
    ds->_mem = base;
    ds->_mem_len = file_size;

    fprintf(stderr, "Loaded %zu references from %s (%.1f MB, 8-bit quantized)\n",
            ds->count, path, file_size / (1024.0 * 1024.0));

    return 0;
}

void dataset_free(struct dataset *ds) {
    if (ds && ds->_mem) {
        munmap(ds->_mem, ds->_mem_len);
        ds->entries = NULL;
        ds->count = 0;
        ds->_mem = NULL;
        ds->_mem_len = 0;
    }
}
