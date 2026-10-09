#ifndef STFT_IMG_H  
#define STFT_IMG_H
#include <stdint.h>
#include "config.h"

/* Image <-> Signal conversion via ISTFT/STFT with no overlap.
 * See EXPLANATION.md Section 3. */

/* Convert a normalized grayscale image to a target signal.
 * img: row-major float image, height H, width W, values in [0,1].
 * signal: output, W*N floats.
 * twiddles: precomputed FFT twiddles for size N.
 * window: Kaiser window of length N.
 * work: temporary buffer, at least 2*N floats (N complex values).
 * N: FFT size (2*H).
 * beta: Kaiser beta (for documentation; window already computed).
 * epsilon: window division clamp threshold.
 * Returns target signal length = W * N. */
stego_status_t image_to_signal(const float *img, uint32_t H, uint32_t W,
                                float *signal, uint32_t *signal_len,
                                const float *twiddles, const float *window,
                                float *work, uint32_t N, float epsilon);

/* Convert a target signal back to a grayscale image.
 * signal: W*N floats.
 * img: output, row-major float image, height H, width W.
 * twiddles: precomputed FFT twiddles for size N.
 * window: Kaiser window of length N.
 * work: temporary buffer, at least 2*N floats.
 * N: FFT size (2*H). */
stego_status_t signal_to_image(const float *signal, uint32_t signal_len,
                                float *img, uint32_t H, uint32_t W,
                                const float *twiddles, const float *window,
                                float *work, uint32_t N);

#endif
