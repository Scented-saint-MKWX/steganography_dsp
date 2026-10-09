#include "permute.h"
#include "prng.h"
#include <stddef.h>

stego_status_t generate_permutation(uint32_t *perm_out, uint32_t len, uint32_t key) {
    if (perm_out == NULL || len == 0) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    /* Initialize with identity permutation */
    for (uint32_t i = 0; i < len; i++) {
        perm_out[i] = i;
    }

    uint32_t state = key;
    /* Fisher-Yates shuffle */
    for (uint32_t i = len - 1; i > 0; i--) {
        uint32_t j = bounded_random(&state, i + 1);
        uint32_t tmp = perm_out[i];
        perm_out[i] = perm_out[j];
        perm_out[j] = tmp;
    }

    return STEGO_STATUS_OK;
}

stego_status_t apply_permutation(const float *in, float *out,
                                  const uint32_t *perm, uint32_t len) {
    if (in == NULL || out == NULL || perm == NULL) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    /* Scatter */
    for (uint32_t i = 0; i < len; i++) {
        out[perm[i]] = in[i];
    }

    return STEGO_STATUS_OK;
}

stego_status_t apply_inverse_permutation(const float *in, float *out,
                                          const uint32_t *perm, uint32_t len) {
    if (in == NULL || out == NULL || perm == NULL) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    /* Gather */
    for (uint32_t i = 0; i < len; i++) {
        out[i] = in[perm[i]];
    }

    return STEGO_STATUS_OK;
}
