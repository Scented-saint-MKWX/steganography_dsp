#ifndef MIRROR_H
#define MIRROR_H
#include <stdint.h>
#include "config.h"

/* Hermitian symmetric spectrum construction ('make pair').
 * See EXPLANATION.md Section 3.1.
 *
 * Takes an H-row image column and produces an N-point complex spectrum
 * (N = 2*H) with Hermitian symmetry so IFFT output is real. */

/* Construct Hermitian-symmetric spectrum from one image column.
 * img_col: H float values (one column of the normalized image).
 * spectrum: output, 2*N floats (N complex values, interleaved re/im).
 * H: image height (number of useful frequency bins).
 * N: FFT size (must be 2*H). */
stego_status_t make_pair_column(const float *img_col, float *spectrum,
                                 uint32_t H, uint32_t N);

/* Reverse of make_pair: extract H pixel values from spectrum.
 * spectrum: 2*N floats (N complex values).
 * img_col: output, H float values.
 * H: image height.
 * N: FFT size (must be 2*H). */
stego_status_t unmake_pair_column(const float *spectrum, float *img_col,
                                   uint32_t H, uint32_t N);

#endif
