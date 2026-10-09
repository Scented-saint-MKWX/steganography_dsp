#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../core/prng.h"

int test_prng_deterministic(void) {
    uint32_t s1 = 0x12345678;
    uint32_t s2 = 0x12345678;
    for (int i = 0; i < 1000; i++) {
        uint32_t v1 = xorshift32(&s1);
        uint32_t v2 = xorshift32(&s2);
        if (v1 != v2) {
            printf("FAIL: test_prng_deterministic mismatch at step %d\n", i);
            return 1;
        }
    }
    printf("PASS: test_prng_deterministic\n");
    return 0;
}

int test_bounded_random_range(void) {
    uint32_t state = 42;
    for (uint32_t n = 1; n <= 100; n++) {
        for (int i = 0; i < 50; i++) {
            uint32_t val = bounded_random(&state, n);
            if (val >= n) {
                printf("FAIL: test_bounded_random_range val %u >= bound %u\n", val, n);
                return 1;
            }
        }
    }
    printf("PASS: test_bounded_random_range\n");
    return 0;
}

int test_key_check_hash(void) {
    uint32_t k1 = 0xDEADBEEF;
    uint32_t k2 = 0xDEADBEEF;
    uint32_t k3 = 0x12345678;
    if (key_check_hash(k1) != key_check_hash(k2)) {
        printf("FAIL: test_key_check_hash identical keys gave different hashes\n");
        return 1;
    }
    if (key_check_hash(k1) == key_check_hash(k3)) {
        printf("FAIL: test_key_check_hash different keys gave collision\n");
        return 1;
    }
    printf("PASS: test_key_check_hash\n");
    return 0;
}
