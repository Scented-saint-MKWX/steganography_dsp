#ifndef PRNG_H
#define PRNG_H
#include <stdint.h>

/* xorshift32 PRNG - deterministic, portable, bit-exact across C and Python.
 * See EXPLANATION.md Section 5. */

/* Advance the PRNG state and return the new value. */
uint32_t xorshift32(uint32_t *state);

/* Return an unbiased random value in [0, n-1] using rejection sampling. */
uint32_t bounded_random(uint32_t *state, uint32_t n);

/* Compute a non-cryptographic hash of the key for verification. */
uint32_t key_check_hash(uint32_t key);

#endif
