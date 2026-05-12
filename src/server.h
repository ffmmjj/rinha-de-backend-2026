#ifndef SERVER_H
#define SERVER_H

#include <microhttpd.h>
#include "dataset.h"

/* ──────────────────────────────────────────────
 * Dataset — global, populated on startup
 * ────────────────────────────────────────────── */

extern struct dataset g_dataset;

/* ──────────────────────────────────────────────
 * Server lifecycle
 * ────────────────────────────────────────────── */

struct MHD_Daemon *server_start(unsigned short port, int worker_count);

void server_stop(struct MHD_Daemon *daemon);

#endif /* SERVER_H */
