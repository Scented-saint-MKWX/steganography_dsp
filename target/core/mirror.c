#include "mirror.h"
#include <stddef.h>

stego_status_t make_pair_column(const float *img_col, float *spectrum,
                                 uint32_t H, uint32_t N) {
    if (img_col == NULL || spectrum == NULL || N != 2 * H) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    /* DC bin: real-only */
    spectrum[0] = img_col[0];
    spectrum[1] = 0.0f;

    /* Positive frequencies */
    for (uint32_t k = 1; k < H; k++) {
        spectrum[2 * k]     = img_col[k];
        spectrum[2 * k + 1] = 0.0f;
    }

    /* Nyquist bin: real-only (set to 0 per specification) */
    spectrum[2 * H]     = 0.0f;
    spectrum[2 * H + 1] = 0.0f;

    /* Negative frequencies (Hermitian mirror of positive frequencies).
     * Since the original image values are real, their conjugate is also real.
     * Thus, the imaginary parts remain 0. */
    for (uint32_t k = 1; k < H; k++) {
        spectrum[2 * (N - k)]     = img_col[k];
        spectrum[2 * (N - k) + 1] = 0.0f;
    }

    return STEGO_STATUS_OK;
}

stego_status_t unmake_pair_column(const float *spectrum, float *img_col,
                                   uint32_t H, uint32_t N) {
    if (spectrum == NULL || img_col == NULL || N != 2 * H) {
        return STEGO_STATUS_ERROR_INVALID_ARG;
    }

    /* Extract only the real parts of bins 0 to H-1 */
    for (uint32_t k = 0; k < H; k++) {
        img_col[k] = spectrum[2 * k];
    }

    return STEGO_STATUS_OK;
}
