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
 *   [0..7]  uint64_t  count
 *   [8..]   struct reference[count]
 *             [0..55]  float vector[14]
 *             [56]     uint8 label
 *────────────────────────────────────────────── */

#define HEADER_SIZE 8

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

    size_t expected_size = HEADER_SIZE + count * sizeof(struct reference);
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

    fprintf(stderr, "Loaded %zu references from %s (%.1f MB)\n",
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
