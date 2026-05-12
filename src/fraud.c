#include "fraud.h"
#include <stdlib.h>

bool fraud_detect(const struct transaction *tx) {
    (void)tx;

    /* Randomly return true or false for now */
    return rand() % 2 == 0;
}
