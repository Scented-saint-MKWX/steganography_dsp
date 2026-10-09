/*
 * extract.c - Full extraction pipeline
 *
 * Orchestrates: DWT extract -> unscale -> inverse permute -> signal_to_image
 * See EXPLANATION.md Section 7 for the data flow diagram.
 */
#include "extract.h"
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

stego_status_t stego_extract(extract_params_t *p) {
    if (!p) return STEGO_STATUS_ERR_BAD_ARGS;

    /* 1. Validate inputs and header */
    if (!p->header || !p->stego || !p->image_out) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }
    if (!p->target_buf || !p->perm_buf || !p->fft_twiddles ||
        !p->kaiser_win || !p->fft_work || !p->dwt_work) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }

    const stego_header_t *h = p->header;
    if (h->magic != STEGO_HEADER_MAGIC) {
        return STEGO_STATUS_ERR_BAD_MAGIC;
    }
    if (h->version != STEGO_HEADER_VERSION) {
        return STEGO_STATUS_ERR_BAD_VERSION;
    }

    uint32_t expected_crc = crc32_compute(h, sizeof(stego_header_t) - sizeof(uint32_t));
    if (h->crc32 != expected_crc) {
        return STEGO_STATUS_ERR_BAD_CRC;
    }

    if (h->key_check != key_check_hash(p->key)) {
        return STEGO_STATUS_ERR_BAD_KEY;
    }

    uint32_t H = h->img_height;
    uint32_t W = h->img_width;
    uint32_t N = h->fft_n;
    uint32_t total_pixels = H * W;

    if (total_pixels > p->img_buf_size) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }
    if (h->target_len > p->target_buf_size || h->target_len > p->perm_buf_size) {
        return STEGO_STATUS_ERR_BAD_ARGS;
    }

    stego_status_t st;

    /* 2. Precompute FFT twiddles and Kaiser window from header params */
    st = fft_precompute_twiddles(p->fft_twiddles, N);
    if (st != STEGO_STATUS_OK) return st;

    kaiser_window(p->kaiser_win, N, h->kaiser_beta);

    /* 3. DWT cascade decompose stego audio */
    wavelet_t wav;
    st = wavelet_get(h->wavelet_id, &wav);
    if (st != STEGO_STATUS_OK) return st;

    /* We need to re-decompose the stego audio down to level L */
    uint32_t pad_multiple = 1u << h->dwt_level;
    uint32_t padded_len = ((h->original_carrier_len + pad_multiple - 1) / pad_multiple) * pad_multiple;

    /* Use dwt_work for decomposition bands */
    float *approx_bands[STEGO_MAX_DWT_LEVELS];
    uint32_t approx_lens[STEGO_MAX_DWT_LEVELS];

    float *dwt_mem = p->dwt_work;
    uint32_t est_len = padded_len;
    for (uint32_t l = 0; l < h->dwt_level; l++) {
        est_len /= 2;
        approx_bands[l] = dwt_mem;
        dwt_mem += est_len;
    }

    float *detail_band = dwt_mem;
    dwt_mem += est_len;

    /* Also need a padded copy of stego audio to decompose */
    float *stego_padded = dwt_mem;
    dwt_mem += padded_len;

    float *dwt_tmp = dwt_mem;
    uint32_t remaining_work = p->dwt_work_size - (uint32_t)(dwt_tmp - p->dwt_work);

    /* Copy and pad stego audio */
    memcpy(stego_padded, p->stego, (p->stego_len < padded_len ? p->stego_len : padded_len) * sizeof(float));
    for (uint32_t i = p->stego_len; i < padded_len; i++) {
        stego_padded[i] = 0.0f;
    }

    uint32_t detail_len = 0;
    st = dwt_cascade_decompose(stego_padded, padded_len,
                                detail_band, &detail_len,
                                h->dwt_level, &wav,
                                dwt_tmp, remaining_work,
                                approx_bands, approx_lens);
    if (st != STEGO_STATUS_OK) return st;

    if (h->target_len > detail_len) {
        return STEGO_STATUS_ERR_CARRIER_SHORT;
    }

    /* 4. Extract first target_len samples from deepest detail band */
    memcpy(p->target_buf, detail_band, h->target_len * sizeof(float));

    /* 5. Divide by scale factor */
    if (h->scale_factor != 0.0f) {
        float inv_scale = 1.0f / h->scale_factor;
        for (uint32_t i = 0; i < h->target_len; i++) {
            p->target_buf[i] *= inv_scale;
        }
    }

    /* 6. Generate permutation with key, apply inverse permutation */
    st = generate_permutation(p->perm_buf, h->target_len, p->key);
    if (st != STEGO_STATUS_OK) return st;

    /* Apply inverse permutation using temporary buffer */
    {
        float *unperm_temp = p->dwt_work;
        st = apply_inverse_permutation(p->target_buf, unperm_temp,
                                       p->perm_buf, h->target_len);
        if (st != STEGO_STATUS_OK) return st;
        memcpy(p->target_buf, unperm_temp, h->target_len * sizeof(float));
    }

    /* 7. Signal to image (apply window, FFT, unmake pair) */
    st = signal_to_image(p->target_buf, h->target_len,
                         p->image_out, H, W,
                         p->fft_twiddles, p->kaiser_win,
                         p->fft_work, N);
    if (st != STEGO_STATUS_OK) return st;

    /* 8. Denormalize using img_min, img_max from header */
    float range = h->img_max - h->img_min;
    for (uint32_t i = 0; i < total_pixels; i++) {
        p->image_out[i] = (p->image_out[i] * range) + h->img_min;
    }

    return STEGO_STATUS_OK;
}
