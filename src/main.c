#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "server.h"
#include "dataset.h"

#define PORT 9999
#define DATASET_PATH "resources/references.bin"

/* Global dataset (defined in server.c) */
extern struct dataset g_dataset;

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    if (dataset_load(DATASET_PATH, &g_dataset) != 0) {
        fprintf(stderr, "Failed to load dataset from %s\n", DATASET_PATH);
        return EXIT_FAILURE;
    }

    if (server_start(PORT) != 0) {
        fprintf(stderr, "Failed to start h2o server on port %d\n", PORT);
        dataset_free(&g_dataset);
        return EXIT_FAILURE;
    }

    /* h2o runs the event loop inside server_start() —
     * it never returns until SIGINT/SIGTERM. */
    dataset_free(&g_dataset);
    return EXIT_SUCCESS;
}
