#include <stdio.h>
#include <math.h>
#include "../core/dwt.h"

int test_dwt_haar_roundtrip(void) {
    uint32_t len = 64;
    float in[64];
    float approx[32];
    float detail[32];
    float out[64];
    float work[128];
    uint32_t out_len = 0;

    wavelet_t wav;
    wavelet_get(STEGO_WAVELET_HAAR, &wav);

    for (uint32_t i = 0; i < len; i++) {
        in[i] = sinf((float)i * 0.2f);
    }

    dwt_single(in, len, approx, detail, &out_len, &wav, work);
    idwt_single(approx, detail, out_len, out, &out_len, &wav, work);

    float max_err = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        float err = fabsf(in[i] - out[i]);
        if (err > max_err) max_err = err;
    }

    if (max_err > 1e-5f) {
        printf("FAIL: test_dwt_haar_roundtrip max error = %e\n", max_err);
        return 1;
    }

    printf("PASS: test_dwt_haar_roundtrip (max err = %e)\n", max_err);
    return 0;
}

int test_dwt_cascade_roundtrip(void) {
    uint32_t len = 128;
    float in[128];
    float out[128];
    float work[512];
    float detail_band[64];
    uint32_t detail_len = 0;
    uint32_t levels = 3;

    wavelet_t wav;
    wavelet_get(STEGO_WAVELET_HAAR, &wav);

    float *approx_bands[3];
    uint32_t approx_lens[3];
    float approx_mem[128];
    approx_bands[0] = &approx_mem[0];
    approx_bands[1] = &approx_mem[64];
    approx_bands[2] = &approx_mem[96];

    for (uint32_t i = 0; i < len; i++) in[i] = (float)i * 0.5f;

    dwt_cascade_decompose(in, len, detail_band, &detail_len, levels, &wav, work, 512, approx_bands, approx_lens);
    uint32_t recon_len = 0;
    dwt_cascade_reconstruct(detail_band, detail_len, out, &recon_len, levels, &wav, work, 512, approx_bands, approx_lens);

    float max_err = 0.0f;
    for (uint32_t i = 0; i < len; i++) {
        float err = fabsf(in[i] - out[i]);
        if (err > max_err) max_err = err;
    }

    if (max_err > 1e-4f) {
        printf("FAIL: test_dwt_cascade_roundtrip max error = %e\n", max_err);
        return 1;
    }

    printf("PASS: test_dwt_cascade_roundtrip (max err = %e)\n", max_err);
    return 0;
}
