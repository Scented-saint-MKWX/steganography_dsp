#include "fft.h"
#include <math.h>
#include <stddef.h>

#ifdef USE_DSPLIB
#include <ti/dsplib/dsplib.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

int is_power_of_2(uint32_t n) {
    return (n != 0) && ((n & (n - 1)) == 0);
}

stego_status_t fft_precompute_twiddles(float *twiddles, uint32_t N) {
    if (twiddles == NULL || !is_power_of_2(N)) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }
    
    for (uint32_t k = 0; k < N; k++) {
        float angle = -2.0f * (float)M_PI * k / N;
        twiddles[2*k]   = cosf(angle);
        twiddles[2*k+1] = sinf(angle);
    }
    return STEGO_STATUS_OK;
}

static void bit_reverse_complex(float *data, uint32_t N) {
    uint32_t j = 0;
    for (uint32_t i = 0; i < N - 1; i++) {
        if (i < j) {
            float temp_re = data[2*i];
            float temp_im = data[2*i+1];
            data[2*i]     = data[2*j];
            data[2*i+1]   = data[2*j+1];
            data[2*j]     = temp_re;
            data[2*j+1]   = temp_im;
        }
        uint32_t k = N >> 1;
        while (k <= j) {
            j -= k;
            k >>= 1;
        }
        j += k;
    }
}

stego_status_t fft_forward(float *restrict data, const float *restrict twiddles, uint32_t N) {
    if (data == NULL || twiddles == NULL || !is_power_of_2(N)) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

#ifdef USE_DSPLIB
    /* NOTE: For DSPLIB DSPF_sp_fftSPxSP to work, data and twiddles must be 
     * 8-byte aligned. Additionally, the twiddles must be precomputed using a
     * specialized format (e.g. gen_twiddle_fftSPxSP), not the one above.
     * Ensure these requirements are met before enabling USE_DSPLIB!
     */
    // DSPF_sp_fftSPxSP(N, data, twiddles, data, brev, 4, 0, N);
    // return STEGO_STATUS_OK;
#endif

    bit_reverse_complex(data, N);

    for (uint32_t step = 1; step < N; step <<= 1) {
        uint32_t jump = step << 1;
        uint32_t twiddle_step = N / jump;
        
        for (uint32_t group = 0; group < step; group++) {
            float tw_re = twiddles[2 * group * twiddle_step];
            float tw_im = twiddles[2 * group * twiddle_step + 1];
            
            #ifdef __TI_COMPILER_VERSION__
            #pragma MUST_ITERATE(1,,)
            #endif
            for (uint32_t pair = group; pair < N; pair += jump) {
                uint32_t match = pair + step;
                float d_match_re = data[2*match];
                float d_match_im = data[2*match+1];
                
                float tr = tw_re * d_match_re - tw_im * d_match_im;
                float ti = tw_re * d_match_im + tw_im * d_match_re;
                
                data[2*match]   = data[2*pair] - tr;
                data[2*match+1] = data[2*pair+1] - ti;
                data[2*pair]   += tr;
                data[2*pair+1] += ti;
            }
        }
    }
    
    return STEGO_STATUS_OK;
}

stego_status_t fft_inverse(float *restrict data, const float *restrict twiddles, uint32_t N) {
    if (data == NULL || twiddles == NULL || !is_power_of_2(N)) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }
    
    /* Conjugate input */
    #ifdef __TI_COMPILER_VERSION__
    #pragma MUST_ITERATE(1,,)
    #endif
    for (uint32_t i = 0; i < N; i++) {
        data[2*i+1] = -data[2*i+1];
    }
    
    stego_status_t status = fft_forward(data, twiddles, N);
    if (status != STEGO_STATUS_OK) {
        return status;
    }
    
    /* Conjugate output and scale */
    float scale = 1.0f / N;
    #ifdef __TI_COMPILER_VERSION__
    #pragma MUST_ITERATE(1,,)
    #endif
    for (uint32_t i = 0; i < N; i++) {
        data[2*i]   *= scale;
        data[2*i+1] = -data[2*i+1] * scale;
    }
    
    return STEGO_STATUS_OK;
}
