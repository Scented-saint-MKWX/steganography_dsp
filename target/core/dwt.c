#include "core/dwt.h"
#include <math.h>
#include <string.h>

/* Haar wavelet */
static const float haar_lpf_d[2] = { 0.7071067811865475f, 0.7071067811865475f };
static const float haar_hpf_d[2] = { 0.7071067811865475f, -0.7071067811865475f };
static const float haar_lpf_r[2] = { 0.7071067811865475f, 0.7071067811865475f };
static const float haar_hpf_r[2] = { -0.7071067811865475f, 0.7071067811865475f };

/* db2 wavelet */
static const float db2_lpf_d[4] = { 0.4829629131f, 0.8365163037f, 0.2241438680f, -0.1294095226f };
static const float db2_hpf_d[4] = { -0.1294095226f, -0.2241438680f, 0.8365163037f, -0.4829629131f };
static const float db2_lpf_r[4] = { -0.1294095226f, 0.2241438680f, 0.8365163037f, 0.4829629131f };
static const float db2_hpf_r[4] = { -0.4829629131f, 0.8365163037f, -0.2241438680f, -0.1294095226f };

stego_status_t wavelet_get(uint32_t wavelet_id, wavelet_t *wav) {
    if (!wav) return STEGO_ERR_NULL_PTR;
    
    if (wavelet_id == STEGO_WAVELET_HAAR) {
        wav->lpf_d = haar_lpf_d;
        wav->hpf_d = haar_hpf_d;
        wav->lpf_r = haar_lpf_r;
        wav->hpf_r = haar_hpf_r;
        wav->filt_len = 2;
        return STEGO_STATUS_OK;
    } else if (wavelet_id == STEGO_WAVELET_DB2) {
        wav->lpf_d = db2_lpf_d;
        wav->hpf_d = db2_hpf_d;
        wav->lpf_r = db2_lpf_r;
        wav->hpf_r = db2_hpf_r;
        wav->filt_len = 4;
        return STEGO_STATUS_OK;
    }
    
    return STEGO_ERR_INVALID_PARAM;
}

stego_status_t dwt_single(const float *restrict input, uint32_t len,
                           float *restrict approx, float *restrict detail, uint32_t *out_len,
                           const wavelet_t *wav, float *work) {
    if (!input || !approx || !detail || !out_len || !wav || !work) return STEGO_ERR_NULL_PTR;
    if (len % 2 != 0) return STEGO_ERR_INVALID_PARAM;
    
    *out_len = len / 2;
    
    if (wav->filt_len == 2) { // Haar optimized
        const float inv_sqrt2 = 0.7071067811865475f;
        for (uint32_t k = 0; k < *out_len; k++) {
            float a = input[2*k];
            float b = input[2*k + 1];
            approx[k] = (a + b) * inv_sqrt2;
            detail[k] = (a - b) * inv_sqrt2;
        }
    } else {
        // General convolution-based DWT with symmetric extension
        uint32_t f_len = wav->filt_len;
        for (uint32_t k = 0; k < *out_len; k++) {
            float sum_a = 0.0f;
            float sum_d = 0.0f;
            for (uint32_t i = 0; i < f_len; i++) {
                int32_t idx = 2*k + i - (f_len / 2) + 1; // centering
                // Symmetric extension for boundaries
                if (idx < 0) idx = -idx - 1;
                else if (idx >= (int32_t)len) idx = 2 * len - 1 - idx;
                
                sum_a += input[idx] * wav->lpf_d[i];
                sum_d += input[idx] * wav->hpf_d[i];
            }
            approx[k] = sum_a;
            detail[k] = sum_d;
        }
    }
    return STEGO_STATUS_OK;
}

stego_status_t idwt_single(const float *restrict approx, const float *restrict detail,
                            uint32_t coeff_len, float *restrict output,
                            uint32_t *out_len, const wavelet_t *wav,
                            float *work) {
    if (!approx || !detail || !output || !out_len || !wav || !work) return STEGO_ERR_NULL_PTR;
    
    *out_len = 2 * coeff_len;
    
    if (wav->filt_len == 2) { // Haar optimized
        const float inv_sqrt2 = 0.7071067811865475f;
        for (uint32_t k = 0; k < coeff_len; k++) {
            float a = approx[k];
            float d = detail[k];
            output[2*k]   = (a + d) * inv_sqrt2;
            output[2*k+1] = (a - d) * inv_sqrt2;
        }
    } else {
        uint32_t f_len = wav->filt_len;
        for (uint32_t i = 0; i < *out_len; i++) {
            float sum = 0.0f;
            // Upsample and filter
            for (uint32_t j = 0; j < f_len; j++) {
                int32_t k = (i - j + (f_len / 2)) / 2;
                if ((i - j + (f_len / 2)) % 2 == 0) {
                    if (k < 0) k = -k - 1;
                    else if (k >= (int32_t)coeff_len) k = 2 * coeff_len - 1 - k;
                    sum += approx[k] * wav->lpf_r[j] + detail[k] * wav->hpf_r[j];
                }
            }
            output[i] = sum;
        }
    }
    return STEGO_STATUS_OK;
}

stego_status_t dwt_cascade_decompose(
    const float *input, uint32_t len,
    float *output, uint32_t *out_len,
    uint32_t levels, const wavelet_t *wav,
    float *work, uint32_t work_size,
    float **approx_bands, uint32_t *approx_lens) {
    
    if (!input || !output || !out_len || !wav || !work || !approx_bands || !approx_lens) return STEGO_ERR_NULL_PTR;
    if (levels == 0) return STEGO_ERR_INVALID_PARAM;
    (void)work_size;
    
    const float *current_in = input;
    uint32_t current_len = len;
    
    for (uint32_t l = 0; l < levels; l++) {
        uint32_t next_len;
        stego_status_t st = dwt_single(current_in, current_len, approx_bands[l], output, &next_len, wav, work);
        if (st != STEGO_STATUS_OK) return st;
        approx_lens[l] = next_len;
        current_len = next_len;
        current_in = output; // Output of detail becomes input for next
    }
    *out_len = current_len;
    return STEGO_STATUS_OK;
}

stego_status_t dwt_cascade_reconstruct(
    const float *detail_band, uint32_t detail_len,
    float *output, uint32_t *out_len,
    uint32_t levels, const wavelet_t *wav,
    float *work, uint32_t work_size,
    float * const *approx_bands, const uint32_t *approx_lens) {
    
    if (!detail_band || !output || !out_len || !wav || !work || !approx_bands || !approx_lens) return STEGO_ERR_NULL_PTR;
    if (levels == 0) return STEGO_ERR_INVALID_PARAM;
    
    const float *current_detail = detail_band;
    uint32_t current_len = detail_len;
    float *current_out = output;
    
    // We need a temp buffer to hold intermediate reconstructions
    float *temp = work;
    if (levels > 1) current_out = temp;
    
    (void)work_size;
    for (int32_t l = (int32_t)levels - 1; l >= 0; l--) {
        uint32_t next_len = 0;
        if (l == 0) current_out = output;
        stego_status_t st = idwt_single(approx_bands[l], current_detail, current_len, current_out, &next_len, wav, work);
        if (st != STEGO_STATUS_OK) return st;
        current_len = next_len;
        if (l > 0) {
            // swap current_detail and current_out for next iteration, but current_out is already temp
            // so we copy back or toggle buffers. Simple way is copy.
            memcpy(work + next_len, current_out, next_len * sizeof(float));
            current_detail = work + next_len;
        }
    }
    *out_len = current_len;
    return STEGO_STATUS_OK;
}

uint32_t dwt_compute_level(uint32_t carrier_len, uint32_t target_len) {
    uint32_t L = 0;
    uint32_t temp = carrier_len;
    while (temp / 2 >= target_len && L < STEGO_MAX_DWT_LEVELS) { 
        temp /= 2; 
        L++; 
    }
    return L;
}
