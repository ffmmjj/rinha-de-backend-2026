#include "features.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ──────────────────────────────────────────────
 * Normalization constants
 * ────────────────────────────────────────────── */

const struct norm_constants g_norm = {
    .max_amount = 10000.0,
    .max_installments = 12.0,
    .amount_vs_avg_ratio = 10.0,
    .max_minutes = 1440.0,
    .max_km = 1000.0,
    .max_tx_count_24h = 20.0,
    .max_merchant_avg_amount = 10000.0,
};

/* ──────────────────────────────────────────────
 * Helpers
 * ────────────────────────────────────────────── */

static double limit(double x) {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

/*
 * Parse ISO 8601: "2026-05-11T12:00:00Z"
 * Returns hour (0-23) and day-of-week (0=Monday, 6=Sunday).
 */
static int parse_iso_8601(const char *s, int *hour, int *day_of_week) {
    int year, month, day, h, m, sec;
    if (sscanf(s, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &h, &m, &sec) < 6)
        return -1;

    *hour = h;

    /* Zeller-like / Tomohiko Sakamoto algorithm for day of week */
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (month < 3) year--;
    *day_of_week = (year + year/4 - year/100 + year/400 + t[month - 1] + day) % 7;
    /* 0 = Monday, 6 = Sunday */
    return 0;
}

/*
 * Compute minutes between two ISO 8601 timestamps.
 * Returns minutes as a double, or -1 on error.
 */
static double minutes_between(const char *t1, const char *t2) {
    struct tm tm1, tm2;
    int sec1, sec2;

    if (sscanf(t1, "%d-%d-%dT%d:%d:%d",
               &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday,
               &tm1.tm_hour, &tm1.tm_min, &sec1) < 6) return -1;
    if (sscanf(t2, "%d-%d-%dT%d:%d:%d",
               &tm2.tm_year, &tm2.tm_mon, &tm2.tm_mday,
               &tm2.tm_hour, &tm2.tm_min, &sec2) < 6) return -1;

    tm1.tm_year -= 1900;
    tm1.tm_mon -= 1;
    tm1.tm_sec = sec1;
    tm2.tm_year -= 1900;
    tm2.tm_mon -= 1;
    tm2.tm_sec = sec2;

    time_t t1s = timegm(&tm1);
    time_t t2s = timegm(&tm2);
    if (t1s == (time_t)-1 || t2s == (time_t)-1) return -1;

    return difftime(t1s, t2s) / 60.0;
}

/*
 * Check if merchant_id is in known_merchants array.
 */
static bool is_known_merchant(const struct transaction *tx) {
    for (size_t i = 0; i < tx->customer.known_merchants_count; i++) {
        if (strcmp(tx->merchant.id, tx->customer.known_merchants[i]) == 0)
            return true;
    }
    return false;
}

/* ──────────────────────────────────────────────
 * Feature extraction
 * ────────────────────────────────────────────── */

void features_extract(const struct transaction *tx, float out[VECTOR_LEN]) {
    /* 0: amount */
    out[0] = (float)limit(tx->transaction.amount / g_norm.max_amount);

    /* 1: installments */
    out[1] = (float)limit((double)tx->transaction.installments / g_norm.max_installments);

    /* 2: amount_vs_avg */
    double ratio = (tx->customer.avg_amount > 0.0)
        ? tx->transaction.amount / tx->customer.avg_amount
        : 0.0;
    out[2] = (float)limit(ratio / g_norm.amount_vs_avg_ratio);

    /* 3: hour_of_day */
    int hour = 0, day_of_week = 0;
    if (parse_iso_8601(tx->transaction.requested_at, &hour, &day_of_week) == 0) {
        out[3] = (float)((double)hour / 23.0);
    } else {
        out[3] = 0.0f;
    }

    /* 4: day_of_week */
    out[4] = (float)((double)day_of_week / 6.0);

    /* 5: minutes_since_last_tx */
    if (tx->last) {
        double mins = minutes_between(tx->transaction.requested_at,
                                       tx->last->timestamp);
        if (mins >= 0.0) {
            out[5] = (float)limit(mins / g_norm.max_minutes);
        } else {
            out[5] = -1.0f; /* error computing minutes */
        }
    } else {
        out[5] = -1.0f;
    }

    /* 6: km_from_last_tx */
    if (tx->last) {
        out[6] = (float)limit(tx->last->km_from_current / g_norm.max_km);
    } else {
        out[6] = -1.0f;
    }

    /* 7: km_from_home */
    out[7] = (float)limit(tx->terminal.km_from_home / g_norm.max_km);

    /* 8: tx_count_24h */
    out[8] = (float)limit((double)tx->customer.tx_count_24h / g_norm.max_tx_count_24h);

    /* 9: is_online */
    out[9] = tx->terminal.is_online ? 1.0f : 0.0f;

    /* 10: card_present */
    out[10] = tx->terminal.card_present ? 1.0f : 0.0f;

    /* 11: unknown_merchant (1 = unknown) */
    out[11] = is_known_merchant(tx) ? 0.0f : 1.0f;

    /* 12: mcc_risk — hardcoded default 0.5 for now */
    out[12] = 0.5f;

    /* 13: merchant_avg_amount */
    out[13] = (float)limit(tx->merchant.avg_amount / g_norm.max_merchant_avg_amount);
}
