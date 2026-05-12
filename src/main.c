#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

#define PORT 9999
#define WORKER_COUNT 4

#define DATASET_PATH "resources/references.bin"

int main(void) {
    if (dataset_load(DATASET_PATH, &g_dataset) != 0) {
        fprintf(stderr, "Failed to load dataset from %s\n", DATASET_PATH);
        return EXIT_FAILURE;
    }

    struct MHD_Daemon *daemon = server_start(PORT, WORKER_COUNT);
    if (daemon == NULL) {
        fprintf(stderr, "Failed to start daemon on port %d\n", PORT);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "Listening on http://0.0.0.0:%d (%d workers)\n",
            PORT, WORKER_COUNT);

    /* Run until killed */
    for (;;) {
        sleep(1);
    }

    server_stop(daemon);
    dataset_free(&g_dataset);
    return EXIT_SUCCESS;
}
