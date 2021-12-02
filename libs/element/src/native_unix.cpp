
#include "element/timestamp.h"

uint64_t element_time_ns()
{
	struct timespec ts;
	clock_gettime (CLOCK_MONOTONIC, &ts);
	return ((uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec);
}
