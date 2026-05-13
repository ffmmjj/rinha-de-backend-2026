#ifndef SERVER_H2O_H
#define SERVER_H2O_H

/* ──────────────────────────────────────────────
 * h2o-based server
 *
 * Starts an HTTP/1.1 server on the given port using
 * h2o's event-loop and handler infrastructure.
 *
 * Blocks forever (event loop runs until killed).
 * Returns 0 on success, -1 on failure.
 * ────────────────────────────────────────────── */

int server_h2o_start(unsigned short port);

#endif /* SERVER_H2O_H */
