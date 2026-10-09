#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../core/fft.h"

int test_fft_roundtrip(void) {
    uint32_t N = 64;
    float twiddles[128];
    float data[128];
    float original[128];

    fft_precompute_twiddles(twiddles, N);

    /* Generate sinusoidal test signal */
    for (uint32_t i = 0; i < N; i++) {
        data[2 * i] = sinf(2.0f * 3.14159265f * 3.0f * (float)i / (float)N);
        data[2 * i + 1] = 0.0f;
        original[2 * i] = data[2 * i];
        original[2 * i + 1] = 0.0f;
    }

    fft_forward(data, twiddles, N);
    fft_inverse(data, twiddles, N);

    float max_err = 0.0f;
    for (uint32_t i = 0; i < 2 * N; i++) {
        float err = fabsf(data[i] - original[i]);
        if (err > max_err) max_err = err;
    }

    if (max_err > 1e-5f) {
        printf("FAIL: test_fft_roundtrip max error = %e\n", max_err);
        return 1;
    }

    printf("PASS: test_fft_roundtrip (max err = %e)\n", max_err);
    return 0;
}

int test_fft_impulse_response(void) {
    uint32_t N = 16;
    float twiddles[32];
    float data[32];

    fft_precompute_twiddles(twiddles, N);

    /* Delta at t=0 */
    memset(data, 0, sizeof(data));
    data[0] = 1.0f;

    fft_forward(data, twiddles, N);

    /* Real part should be 1.0 across all frequency bins, imag should be 0 */
    for (uint32_t i = 0; i < N; i++) {
        if (fabsf(data[2 * i] - 1.0f) > 1e-5f || fabsf(data[2 * i + 1]) > 1e-5f) {
            printf("FAIL: test_fft_impulse_response at bin %u: (%f, %f)\n", i, data[2 * i], data[2 * i + 1]);
            return 1;
        }
    }

    printf("PASS: test_fft_impulse_response\n");
    return 0;
}
