#ifndef PERMUTE_H
#define PERMUTE_H
#include <stdint.h>
#include "config.h"

/* Fisher-Yates permutation driven by xorshift32 PRNG.
 * See EXPLANATION.md Section 5. */

/* Generate a permutation of [0..len-1] into perm_out.
 * Uses Fisher-Yates shuffle seeded with key.
 * perm_out must hold at least len uint32_t values. */
stego_status_t generate_permutation(uint32_t *perm_out, uint32_t len, uint32_t key);

/* Apply permutation: out[perm[i]] = in[i] for i in [0..len-1].
 * in and out must not overlap. */
stego_status_t apply_permutation(const float *in, float *out,
                                  const uint32_t *perm, uint32_t len);

/* Apply inverse permutation: out[i] = in[perm[i]] for i in [0..len-1].
 * in and out must not overlap. */
stego_status_t apply_inverse_permutation(const float *in, float *out,
                                          const uint32_t *perm, uint32_t len);

#endif
