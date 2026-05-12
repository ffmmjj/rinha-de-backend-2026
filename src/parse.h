#ifndef PARSE_H
#define PARSE_H

#include <stdbool.h>
#include <stddef.h>

/* ──────────────────────────────────────────────
 * Minimal JSON string value extractor.
 *
 * Scans @a json for the first occurrence of a key
 * whose suffix matches @a key (e.g. "transaction.amount"
 * matches both "transaction.amount" and, after flattening,
 * just "amount" if unique).
 *
 * For simplicity, we use the full dotted path.
 *
 * Returns a newly allocated string, or NULL if not found.
 * Caller must free.
 * ────────────────────────────────────────────── */

char *json_get_string(const char *json, size_t len,
                       const char *key);

double json_get_number(const char *json, size_t len,
                        const char *key);

int json_get_int(const char *json, size_t len,
                  const char *key);

bool json_get_bool(const char *json, size_t len,
                    const char *key);

/* Returns number of strings found, up to max_count.
   Strings are allocated and stored in out[]. */
size_t json_get_string_array(const char *json, size_t len,
                              const char *key,
                              char **out, size_t max_count);

#endif /* PARSE_H */
