#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../core/permute.h"
#include "../config.h"

int test_permute_roundtrip(void) {
    uint32_t len = 256;
    uint32_t perm[256];
    float in[256];
    float scrambled[256];
    float recovered[256];
    uint32_t key = 0xABCD1234;

    for (uint32_t i = 0; i < len; i++) {
        in[i] = (float)i * 1.5f;
    }

    stego_status_t st = generate_permutation(perm, len, key);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: generate_permutation returned %d\n", st);
        return 1;
    }

    st = apply_permutation(in, scrambled, perm, len);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: apply_permutation returned %d\n", st);
        return 1;
    }

    st = apply_inverse_permutation(scrambled, recovered, perm, len);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: apply_inverse_permutation returned %d\n", st);
        return 1;
    }

    for (uint32_t i = 0; i < len; i++) {
        if (in[i] != recovered[i]) {
            printf("FAIL: permute mismatch at index %u: expected %f, got %f\n", i, in[i], recovered[i]);
            return 1;
        }
    }

    printf("PASS: test_permute_roundtrip\n");
    return 0;
}

int test_permute_wrong_key(void) {
    uint32_t len = 128;
    uint32_t perm_embed[128];
    uint32_t perm_wrong[128];
    float in[128];
    float scrambled[128];
    float recovered[128];

    for (uint32_t i = 0; i < len; i++) in[i] = (float)i;

    generate_permutation(perm_embed, len, 0x11112222);
    generate_permutation(perm_wrong, len, 0x99998888);

    apply_permutation(in, scrambled, perm_embed, len);
    apply_inverse_permutation(scrambled, recovered, perm_wrong, len);

    int diff_count = 0;
    for (uint32_t i = 0; i < len; i++) {
        if (in[i] != recovered[i]) diff_count++;
    }

    if (diff_count < 100) {
        printf("FAIL: test_permute_wrong_key did not sufficiently scramble data (diff_count=%d)\n", diff_count);
        return 1;
    }

    printf("PASS: test_permute_wrong_key\n");
    return 0;
}
