#ifndef DWT_H
#define DWT_H
#include <stdint.h>
#include "config.h"

/* Discrete Wavelet Transform (1D, single level).
 * Supports Haar and db2 wavelets behind a clean interface.
 * See EXPLANATION.md Section 4. */

typedef struct {
    const float *lpf_d;  /* Low-pass decomposition filter */
    const float *hpf_d;  /* High-pass decomposition filter */
    const float *lpf_r;  /* Low-pass reconstruction filter */
    const float *hpf_r;  /* High-pass reconstruction filter */
    uint32_t filt_len;   /* Filter length */
} wavelet_t;

/* Get wavelet by ID (STEGO_WAVELET_HAAR or STEGO_WAVELET_DB2). */
stego_status_t wavelet_get(uint32_t wavelet_id, wavelet_t *wav);

/* Single-level DWT decomposition.
 * input: signal of length len.
 * approx: output low-pass (approximation) coefficients, length ceil(len/2).
 * detail: output high-pass (detail) coefficients, length ceil(len/2).
 * work: temporary buffer, at least len + 2*filt_len floats.
 * Returns the length of each output (= ceil(len/2) for Haar). 
 * NOTE: For Haar, output length = floor(len/2). Pad input to even length first. */
stego_status_t dwt_single(const float *input, uint32_t len,
                           float *approx, float *detail, uint32_t *out_len,
                           const wavelet_t *wav, float *work);

/* Single-level IDWT reconstruction.
 * approx: low-pass coefficients, length coeff_len.
 * detail: high-pass coefficients, length coeff_len.
 * output: reconstructed signal, length 2*coeff_len.
 * work: temporary buffer, at least 4*coeff_len floats. */
stego_status_t idwt_single(const float *approx, const float *detail,
                            uint32_t coeff_len, float *output,
                            uint32_t *out_len, const wavelet_t *wav,
                            float *work);

/* Multi-level DWT down the HIGH-PASS (detail) branch only.
 * This is the paper's cascade: Original -> H -> H -> H ...
 * At each level, take the DETAIL coefficients and feed them to the next level.
 *
 * input: signal of length len.
 * output: the detail coefficients at the final level.
 * out_len: length of the output.
 * levels: number of decomposition levels.
 * wav: wavelet to use.
 * work: large temporary buffer.
 * work_size: size of work buffer in floats.
 *
 * Also stores intermediate detail bands for reconstruction.
 * intermediate: array of buffers for each level's complementary (approx) coefficients.
 * inter_lens: array of lengths for each level. */
stego_status_t dwt_cascade_decompose(
    const float *input, uint32_t len,
    float *output, uint32_t *out_len,
    uint32_t levels, const wavelet_t *wav,
    float *work, uint32_t work_size,
    float **approx_bands, uint32_t *approx_lens);

/* Multi-level IDWT reconstruction from the high-pass cascade.
 * detail_band: the (modified) detail coefficients at the deepest level.
 * detail_len: length of detail_band.
 * output: reconstructed signal, length = original padded length.
 * levels: number of levels.
 * wav: wavelet.
 * work: temporary buffer.
 * approx_bands: the stored approximation coefficients from decomposition.
 * approx_lens: their lengths. */
stego_status_t dwt_cascade_reconstruct(
    const float *detail_band, uint32_t detail_len,
    float *output, uint32_t *out_len,
    uint32_t levels, const wavelet_t *wav,
    float *work, uint32_t work_size,
    float * const *approx_bands, const uint32_t *approx_lens);

/* Compute optimal DWT level: largest L such that len / 2^L >= target_len. */
uint32_t dwt_compute_level(uint32_t carrier_len, uint32_t target_len);

#endif
