#ifndef FFT_H
#define FFT_H
#include <stdint.h>
#include "config.h"

/* Radix-2 Cooley-Tukey FFT/IFFT, single precision.
 * See EXPLANATION.md Section 3.2.
 *
 * Data format: interleaved [re0, im0, re1, im1, ...]
 * Length N must be a power of 2.
 * Twiddle factors are precomputed for efficiency. */

/* Precompute twiddle factors for FFT of size N.
 * twiddles must hold 2*N floats (N complex values). */
stego_status_t fft_precompute_twiddles(float *twiddles, uint32_t N);

/* In-place radix-2 DIT FFT.
 * data: interleaved complex array of 2*N floats.
 * twiddles: precomputed by fft_precompute_twiddles.
 * N: transform size (must be power of 2). */
stego_status_t fft_forward(float *data, const float *twiddles, uint32_t N);

/* In-place radix-2 DIT IFFT.
 * Same interface as fft_forward.
 * Output is scaled by 1/N. */
stego_status_t fft_inverse(float *data, const float *twiddles, uint32_t N);

/* Check if n is a power of 2. */
int is_power_of_2(uint32_t n);

#endif
