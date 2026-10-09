#include <stdio.h>
#include <math.h>
#include <string.h>
#include "../core/embed.h"
#include "../core/extract.h"
#include "../util/metrics.h"

int test_embed_extract_full_roundtrip(void) {
    /* Test on small 8x8 image with 2048-sample carrier */
    uint32_t H = 8;
    uint32_t W = 8;
    uint32_t N = 16;
    uint32_t carrier_len = 2048;
    uint32_t key = 0xDEADBEEF;
    float scale = 0.01f;
    float beta = 1.0f;
    float epsilon = 1e-7f;

    float img_orig[64];
    for (uint32_t i = 0; i < 64; i++) {
        img_orig[i] = (float)i / 64.0f;
    }

    float carrier[2048];
    for (uint32_t i = 0; i < carrier_len; i++) {
        carrier[i] = 0.2f * sinf(2.0f * 3.14159265f * (float)i / 64.0f);
    }

    float stego[2048];
    stego_header_t header;

    /* Embed working buffers */
    float target_buf[256];
    uint32_t perm_buf[256];
    float twiddles[32];
    float window[16];
    float fft_work[32];
    float dwt_work[8192];

    embed_params_t ep;
    memset(&ep, 0, sizeof(ep));
    ep.image = img_orig;
    ep.img_height = H;
    ep.img_width = W;
    ep.carrier = carrier;
    ep.carrier_len = carrier_len;
    ep.key = key;
    ep.scale = scale;
    ep.kaiser_beta = beta;
    ep.fft_n = N;
    ep.win_epsilon = epsilon;
    ep.wavelet_id = STEGO_WAVELET_HAAR;
    ep.img_min = 0.0f;
    ep.img_max = 1.0f;
    ep.stego_out = stego;
    ep.stego_buf_size = 2048;
    ep.header_out = &header;
    ep.target_buf = target_buf;
    ep.target_buf_size = 256;
    ep.perm_buf = perm_buf;
    ep.perm_buf_size = 256;
    ep.fft_twiddles = twiddles;
    ep.kaiser_win = window;
    ep.fft_work = fft_work;
    ep.dwt_work = dwt_work;
    ep.dwt_work_size = 8192;

    stego_status_t st = stego_embed(&ep);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: stego_embed failed with status %d\n", st);
        return 1;
    }

    /* Extract */
    float img_recon[64];
    extract_params_t xp;
    memset(&xp, 0, sizeof(xp));
    xp.stego = stego;
    xp.stego_len = ep.stego_len;
    xp.header = &header;
    xp.key = key;
    xp.image_out = img_recon;
    xp.img_buf_size = 64;
    xp.target_buf = target_buf;
    xp.target_buf_size = 256;
    xp.perm_buf = perm_buf;
    xp.perm_buf_size = 256;
    xp.fft_twiddles = twiddles;
    xp.kaiser_win = window;
    xp.fft_work = fft_work;
    xp.dwt_work = dwt_work;
    xp.dwt_work_size = 8192;

    st = stego_extract(&xp);
    if (st != STEGO_STATUS_OK) {
        printf("FAIL: stego_extract failed with status %d\n", st);
        return 1;
    }

    float mse = compute_mse(img_orig, img_recon, 64);
    float psnr = compute_psnr(img_orig, img_recon, 64, 1.0f);
    printf("Roundtrip Image MSE: %e, PSNR: %.2f dB\n", mse, psnr);

    if (mse > 1e-4f) {
        printf("FAIL: test_embed_extract_full_roundtrip MSE too high\n");
        return 1;
    }

    /* Test wrong key gives noise */
    float img_wrong[64];
    xp.key = key ^ 0x12345678;
    xp.image_out = img_wrong;
    st = stego_extract(&xp);
    if (st != STEGO_STATUS_ERR_BAD_KEY) {
        printf("FAIL: expected bad key error on extract with wrong key, got %d\n", st);
        return 1;
    }

    printf("PASS: test_embed_extract_full_roundtrip\n");
    return 0;
}
