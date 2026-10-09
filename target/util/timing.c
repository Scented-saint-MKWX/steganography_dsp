#include "util/timing.h"
#include "util/platform.h"

uint64_t timing_start(void) {
    return platform_get_cycles();
}

uint64_t timing_end(uint64_t start) {
    return platform_get_cycles() - start;
}
