#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ──────────────────────────────────────────────
 * Low-level helpers
 * ────────────────────────────────────────────── */

static const char *skip_ws(const char *p, const char *end) {
    while (p < end && isspace((unsigned char)*p)) p++;
    return p;
}

/*
 * For a dotted key like "transaction.amount", we need to navigate
 * into nested objects. This helper locates the value for a given key
 * at the current JSON level, optionally recursing into child objects.
 *
 * The key is tokenized by '.': e.g. "transaction.amount" means
 * find key "transaction" at top level, then find "amount" inside its object.
 */

/* Locate the value after '"key":' starting from position p.
   Returns pointer to the first character of the value, or NULL. */
static const char *find_key_at_level(const char *p, const char *end,
                                      const char *key) {
    size_t klen = strlen(key);
    while (p < end) {
        /* Skip non-quote chars */
        const char *q = (const char *)memchr(p, '"', (size_t)(end - p));
        if (q == NULL) return NULL;

        /* Check if this is our key */
        if ((size_t)(end - q) >= klen + 3 &&
            strncmp(q + 1, key, klen) == 0 &&
            q[klen + 1] == '"') {
            /* Found key. Advance past '"key":' */
            const char *v = q + klen + 2;
            v = skip_ws(v, end);
            if (v >= end || *v != ':') { p = q + 1; continue; }
            v = skip_ws(v + 1, end);
            return v;
        }
        p = q + 1;
    }
    return NULL;
}

/* Navigate a dotted path into nested JSON objects.
   Example: "transaction.amount" -> find "transaction":{...,
   then find "amount": inside that object. */
static const char *navigate_path(const char *json, size_t len,
                                  const char *key) {
    const char *end = json + len;

    /* Duplicate the key so we can tokenize */
    char *path = strdup(key);
    if (path == NULL) return NULL;

    const char *current = json;
    char *saveptr = NULL;
    char *token = strtok_r(path, ".", &saveptr);

    while (token != NULL) {
        current = find_key_at_level(current, end, token);
        if (current == NULL) {
            free(path);
            return NULL;
        }
        token = strtok_r(NULL, ".", &saveptr);
        /* If there's a next token, current should point to a '{' object.
           Advance past it so the next find_key_at_level works inside. */
        if (token != NULL) {
            if (*current != '{') {
                free(path);
                return NULL;
            }
            current = skip_ws(current + 1, end);
        }
    }
    free(path);
    return current;
}

/* ──────────────────────────────────────────────
 * Public API
 * ────────────────────────────────────────────── */

char *json_get_string(const char *json, size_t len,
                       const char *key) {
    const char *val = navigate_path(json, len, key);
    if (val == NULL) return NULL;
    const char *end = json + len;
    if (val >= end || *val != '"') return NULL;

    const char *close = val + 1;
    while (close < end && *close != '"') {
        if (*close == '\\') close++;
        close++;
    }
    if (close >= end) return NULL;

    size_t slen = (size_t)(close - val - 1);
    char *s = (char *)malloc(slen + 1);
    if (s == NULL) return NULL;

    size_t j = 0;
    for (const char *src = val + 1; src < close; src++) {
        if (*src == '\\') {
            src++;
            if (src >= close) break;
            switch (*src) {
                case 'n': s[j++] = '\n'; break;
                case 't': s[j++] = '\t'; break;
                case 'r': s[j++] = '\r'; break;
                case '\\': s[j++] = '\\'; break;
                case '"': s[j++] = '"'; break;
                default: s[j++] = *src; break;
            }
        } else {
            s[j++] = *src;
        }
    }
    s[j] = '\0';
    return s;
}

double json_get_number(const char *json, size_t len,
                        const char *key) {
    const char *val = navigate_path(json, len, key);
    if (val == NULL) return NAN;
    const char *end = json + len;
    const char *p = val;

    /* Check for null */
    if ((size_t)(end - p) >= 4 && strncmp(p, "null", 4) == 0)
        return NAN;

    int neg = 0;
    if (p < end && *p == '-') { neg = 1; p++; }

    double result = 0.0;
    while (p < end && isdigit((unsigned char)*p)) {
        result = result * 10.0 + (double)(*p - '0');
        p++;
    }

    if (p < end && *p == '.') {
        p++;
        double frac = 0.0;
        double div = 1.0;
        while (p < end && isdigit((unsigned char)*p)) {
            frac = frac * 10.0 + (double)(*p - '0');
            div *= 10.0;
            p++;
        }
        result += frac / div;
    }

    return neg ? -result : result;
}

int json_get_int(const char *json, size_t len,
                  const char *key) {
    double v = json_get_number(json, len, key);
    return (isnan(v)) ? 0 : (int)v;
}

bool json_get_bool(const char *json, size_t len,
                    const char *key) {
    const char *val = navigate_path(json, len, key);
    if (val == NULL) return false;
    const char *end = json + len;
    if ((size_t)(end - val) >= 4 && strncmp(val, "true", 4) == 0)
        return true;
    if ((size_t)(end - val) >= 5 && strncmp(val, "false", 5) == 0)
        return false;
    return false;
}

size_t json_get_string_array(const char *json, size_t len,
                              const char *key,
                              char **out, size_t max_count) {
    const char *val = navigate_path(json, len, key);
    if (val == NULL) return 0;
    const char *end = json + len;
    if (val >= end || *val != '[') return 0;

    size_t count = 0;
    const char *p = val + 1;
    p = skip_ws(p, end);

    while (p < end && *p != ']' && count < max_count) {
        if (*p == '"') {
            const char *close = p + 1;
            while (close < end && *close != '"') {
                if (*close == '\\') close++;
                close++;
            }
            size_t slen = (size_t)(close - p - 1);
            out[count] = (char *)malloc(slen + 1);
            if (out[count]) {
                memcpy(out[count], p + 1, slen);
                out[count][slen] = '\0';
                count++;
            }
            p = close + 1;
        } else {
            p++;
        }
        p = skip_ws(p, end);
        if (p < end && *p == ',') {
            p = skip_ws(p + 1, end);
        }
    }
    return count;
}
