#include <stdio.h>
#include <math.h>
#include "../core/stft_img.h"
#include "../core/fft.h"
#include "../core/kaiser.h"

int test_stft_img_roundtrip(void) {
    uint32_t H = 8;
    uint32_t W = 8;
    uint32_t N = 16;
    float beta = 1.0f;
    float epsilon = 1e-7f;

    float img[64];
    float recovered[64];
    float signal[128]; /* W * N = 8 * 16 = 128 */
    uint32_t sig_len = 0;

    float twiddles[32];
    float window[16];
    float work[32];

    fft_precompute_twiddles(twiddles, N);
    kaiser_window(window, N, beta);

    /* Generate synthetic test pattern [0, 1] */
    for (uint32_t i = 0; i < 64; i++) {
        img[i] = (float)i / 64.0f;
    }

    stego_status_t st = image_to_signal(img, H, W, signal, &sig_len, twiddles, window, work, N, epsilon);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: image_to_signal returned %d\n", st);
        return 1;
    }

    st = signal_to_image(signal, sig_len, recovered, H, W, twiddles, window, work, N);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: signal_to_image returned %d\n", st);
        return 1;
    }

    float max_diff = 0.0f;
    for (uint32_t i = 0; i < 64; i++) {
        float diff = fabsf(img[i] - recovered[i]);
        if (diff > max_diff) max_diff = diff;
    }

    if (max_diff > 1e-4f) {
        printf("FAIL: test_stft_img_roundtrip max diff = %e\n", max_diff);
        return 1;
    }

    printf("PASS: test_stft_img_roundtrip (max diff = %e)\n", max_diff);
    return 0;
}
