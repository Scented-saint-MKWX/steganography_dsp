#include "stft_img.h"
#include "mirror.h"
#include "fft.h"
#include "config.h"
#include <math.h>
#include <stddef.h>

#ifndef NDEBUG
#include <assert.h>
#endif

stego_status_t image_to_signal(const float *img, uint32_t H, uint32_t W,
                                float *signal, uint32_t *signal_len,
                                const float *twiddles, const float *window,
                                float *work, uint32_t N, float epsilon) {
    if (img == NULL || signal == NULL || signal_len == NULL || 
        twiddles == NULL || window == NULL || work == NULL || N != 2 * H) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    for (uint32_t j = 0; j < W; j++) {
        /* Directly construct Hermitian spectrum in work buffer */
        /* DC bin */
        work[0] = img[0 * W + j];
        work[1] = 0.0f;

        /* Positive frequencies */
        for (uint32_t k = 1; k < H; k++) {
            float val = img[k * W + j];
            work[2 * k]     = val;
            work[2 * k + 1] = 0.0f;
        }

        /* Nyquist bin (set to 0) */
        work[2 * H]     = 0.0f;
        work[2 * H + 1] = 0.0f;

        /* Negative frequencies (Hermitian mirror) */
        for (uint32_t k = 1; k < H; k++) {
            work[2 * (N - k)]     = img[k * W + j];
            work[2 * (N - k) + 1] = 0.0f;
        }

        /* IFFT to get time-domain frame */
        stego_status_t status = fft_inverse(work, twiddles, N);
        if (status != STEGO_STATUS_OK) return status;

        /* Divide by window (undoing analysis window at overlap=0) */
        for (uint32_t n = 0; n < N; n++) {
            float win_val = window[n];
            float abs_win = win_val > 0.0f ? win_val : -win_val;
            float denom = abs_win > epsilon ? abs_win : epsilon;
            
            signal[j * N + n] = work[2 * n] / denom;
        }
    }

    *signal_len = W * N;
    return STEGO_STATUS_OK;
}

stego_status_t signal_to_image(const float *signal, uint32_t signal_len,
                                float *img, uint32_t H, uint32_t W,
                                const float *twiddles, const float *window,
                                float *work, uint32_t N) {
    if (signal == NULL || img == NULL || twiddles == NULL || 
        window == NULL || work == NULL || N != 2 * H || signal_len < W * N) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    for (uint32_t j = 0; j < W; j++) {
        /* 1. Apply window and pack as complex */
        for (uint32_t n = 0; n < N; n++) {
            work[2 * n]     = signal[j * N + n] * window[n];
            work[2 * n + 1] = 0.0f;
        }

        /* 2. FFT to get frequency domain spectrum */
        stego_status_t status = fft_forward(work, twiddles, N);
        if (status != STEGO_STATUS_OK) return status;

        /* 3. Extract pixels directly into row-major image (no VLA needed) */
        for (uint32_t k = 0; k < H; k++) {
            img[k * W + j] = work[2 * k];
        }
    }

    return STEGO_STATUS_OK;
}
