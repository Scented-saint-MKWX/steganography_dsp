/*
 * embed.c - Full embedding pipeline
 *
 * Orchestrates: image_to_signal -> permute -> scale -> DWT embed
 * See EXPLANATION.md Section 7 for the data flow diagram.
 */
#include "embed.h"
#include "dwt.h"
#include "fft.h"
#include "kaiser.h"
#include "permute.h"
#include "prng.h"
#include "stft_img.h"
#include "mirror.h"
#include "../io/header.h"
#include "../io/crc32.h"
#include "../config.h"
#include <string.h>

stego_status_t stego_embed(embed_params_t *p) {
    if (!p) return STEGO_STATUS_ERR_BAD_ARGS;

    /* 1. Validate inputs */
    if (p->img_height == 0 || p->img_width == 0 ||
        p->carrier_len == 0 || p->fft_n == 0) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }
    if (!p->image || !p->carrier || !p->stego_out || !p->header_out) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }
    if (!p->target_buf || !p->perm_buf || !p->fft_twiddles ||
        !p->kaiser_win || !p->fft_work || !p->dwt_work) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }
    if (p->fft_n != 2 * p->img_height) {
        return STEGO_STATUS_ERR_FFT_SIZE;
    }

    stego_status_t st;

    /* 2. Precompute FFT twiddles and Kaiser window */
    st = fft_precompute_twiddles(p->fft_twiddles, p->fft_n);
    if (st != STEGO_STATUS_OK) return st;

    kaiser_window(p->kaiser_win, p->fft_n, p->kaiser_beta);

    /* 3. Image to target signal (ISTFT with window inverse) */
    st = image_to_signal(p->image, p->img_height, p->img_width,
                         p->target_buf, &p->target_len,
                         p->fft_twiddles, p->kaiser_win,
                         p->fft_work, p->fft_n, p->win_epsilon);
    if (st != STEGO_STATUS_OK) return st;

    if (p->target_len > p->target_buf_size) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }

    /* 4. Generate permutation and apply it (scatter) */
    st = generate_permutation(p->perm_buf, p->target_len, p->key);
    if (st != STEGO_STATUS_OK) return st;

    /* Need a temp buffer for permutation. Use the first part of dwt_work. */
    {
        float *perm_temp = p->dwt_work;
        st = apply_permutation(p->target_buf, perm_temp,
                               p->perm_buf, p->target_len);
        if (st != STEGO_STATUS_OK) return st;
        /* Copy result back to target_buf */
        memcpy(p->target_buf, perm_temp, p->target_len * sizeof(float));
    }

    /* 5. Scale the permuted signal */
    for (uint32_t i = 0; i < p->target_len; i++) {
        p->target_buf[i] *= p->scale;
    }

    /* 6. Compute DWT level */
    p->dwt_level = dwt_compute_level(p->carrier_len, p->target_len);
    if (p->dwt_level == 0) {
        return STEGO_STATUS_ERR_CARRIER_SHORT;
    }

    /* 7. Pad carrier to multiple of 2^L and copy to stego_out */
    uint32_t pad_multiple = 1u << p->dwt_level;
    uint32_t padded_len = ((p->carrier_len + pad_multiple - 1) / pad_multiple) * pad_multiple;
    if (padded_len > p->stego_buf_size) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }

    memcpy(p->stego_out, p->carrier, p->carrier_len * sizeof(float));
    for (uint32_t i = p->carrier_len; i < padded_len; i++) {
        p->stego_out[i] = 0.0f;
    }
    p->stego_len = padded_len;

    /* 8. DWT cascade decompose */
    wavelet_t wav;
    st = wavelet_get(p->wavelet_id, &wav);
    if (st != STEGO_STATUS_OK) return st;

    /* Allocate approx_bands from dwt_work.
     * Layout in dwt_work: [approx_band_0 | approx_band_1 | ... | detail_out | temp_work] */
    float *approx_bands[STEGO_MAX_DWT_LEVELS];
    uint32_t approx_lens[STEGO_MAX_DWT_LEVELS];

    float *dwt_mem = p->dwt_work;
    uint32_t est_len = padded_len;
    for (uint32_t l = 0; l < p->dwt_level; l++) {
        est_len /= 2;
        approx_bands[l] = dwt_mem;
        dwt_mem += est_len;
    }

    float *detail_band = dwt_mem;
    dwt_mem += est_len;  /* Space for detail output */

    float *dwt_tmp = dwt_mem;
    uint32_t remaining_work = p->dwt_work_size -
                              (uint32_t)(dwt_tmp - p->dwt_work);

    uint32_t detail_len = 0;
    st = dwt_cascade_decompose(p->stego_out, padded_len,
                                detail_band, &detail_len,
                                p->dwt_level, &wav,
                                dwt_tmp, remaining_work,
                                approx_bands, approx_lens);
    if (st != STEGO_STATUS_OK) return st;

    if (p->target_len > detail_len) {
        return STEGO_STATUS_ERR_CARRIER_SHORT;
    }

    /* 9. Overwrite first target_len samples of deepest detail band */
    memcpy(detail_band, p->target_buf, p->target_len * sizeof(float));

    /* 10. DWT cascade reconstruct */
    uint32_t out_len = 0;
    st = dwt_cascade_reconstruct(detail_band, detail_len,
                                  p->stego_out, &out_len,
                                  p->dwt_level, &wav,
                                  dwt_tmp, remaining_work,
                                  approx_bands, approx_lens);
    if (st != STEGO_STATUS_OK) return st;

    p->stego_len = p->carrier_len;  /* Trim to original length */

    /* 11. Fill header */
    stego_header_t *h = p->header_out;
    memset(h, 0, sizeof(stego_header_t));
    h->magic              = STEGO_HEADER_MAGIC;
    h->version            = STEGO_HEADER_VERSION;
    h->img_width          = (uint16_t)p->img_width;
    h->img_height         = (uint16_t)p->img_height;
    h->fft_n              = (uint16_t)p->fft_n;
    h->wavelet_id         = (uint16_t)p->wavelet_id;
    h->kaiser_beta        = p->kaiser_beta;
    h->scale_factor       = p->scale;
    h->img_min            = p->img_min;
    h->img_max            = p->img_max;
    h->dwt_level          = p->dwt_level;
    h->original_carrier_len = p->carrier_len;
    h->target_len         = p->target_len;
    h->key_check          = key_check_hash(p->key);
    /* CRC computed over all preceding bytes */
    h->crc32 = crc32_compute(h, sizeof(stego_header_t) - sizeof(uint32_t));

    return STEGO_STATUS_OK;
}
