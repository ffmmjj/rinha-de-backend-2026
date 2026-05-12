#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

#define PORT 8081
#define WORKER_COUNT 4

int main(void) {
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
    return EXIT_SUCCESS;
}
