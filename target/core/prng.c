#include "prng.h"

uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

uint32_t bounded_random(uint32_t *state, uint32_t n) {
    if (n == 0) return 0;
    uint32_t threshold = (uint32_t)(-n) % n;
    uint32_t r;
    do {
        r = xorshift32(state);
    } while (r < threshold);
    return r % n;
}

uint32_t key_check_hash(uint32_t key) {
    uint32_t h = key;
    h ^= h >> 16;
    h *= 0x45d9f3b;
    h ^= h >> 16;
    return h;
}
