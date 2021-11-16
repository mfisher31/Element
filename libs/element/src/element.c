
#include "element/element.h"
#include <time.h>

uint64_t eds_time_ns() {
    struct timespec ts;
    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}
