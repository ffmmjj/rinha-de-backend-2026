#ifndef SERVER_H
#define SERVER_H

#include <microhttpd.h>

/* ──────────────────────────────────────────────
 * Server lifecycle
 * ────────────────────────────────────────────── */

struct MHD_Daemon *server_start(unsigned short port, int worker_count);

void server_stop(struct MHD_Daemon *daemon);

#endif /* SERVER_H */
