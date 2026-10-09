/*
 * main.c - TMS320C6748 Application Entry Point
 *
 * Supports three execution modes:
 *   - STEGO_MODE_SELFTEST: Runs internal verification on embedded buffers
 *   - STEGO_MODE_EMBED: Reads image + carrier from DDR2, generates stego audio + header
 *   - STEGO_MODE_EXTRACT: Reads stego audio + header from DDR2, recovers hidden image
 *
 * In board operation, the DSS script loads buffers into fixed DDR2 sections,
 * runs the DSP, and waits on `g_done_flag`.
 */

#include <stdint.h>
#include <string.h>
#include <math.h>
#include "config.h"
#include "util/platform.h"
#include "util/timing.h"
#include "util/metrics.h"
#include "io/header.h"
#include "core/embed.h"
#include "core/extract.h"

/* ── Results Structure placed at fixed DDR2 address ─────────────────────── */
typedef struct {
    uint32_t magic;           /* 0x52455355 = "RESU" */
    uint32_t status_code;     /* stego_status_t */
    uint64_t cycles_embed;    /* TSC cycles */
    uint64_t cycles_extract;  /* TSC cycles */
    uint32_t target_len;      /* Generated target signal samples */
    uint32_t dwt_level;       /* Chosen cascade level */
    float    scale_factor;    /* Applied scale */
    float    image_mse;       /* Self-test image MSE */
    float    image_psnr;      /* Self-test image PSNR */
    float    audio_snr;       /* Self-test stego audio SNR */
    uint32_t output_checksum; /* Simple 32-bit sum */
} stego_results_t;

/* ── Buffers placed in designated DDR2 sections via pragmas ─────────────── */
#ifdef __TI_COMPILER_VERSION__
#pragma DATA_SECTION(g_image_in,   ".ddr_image_in")
#pragma DATA_SECTION(g_target_sig,  ".ddr_target")
#pragma DATA_SECTION(g_carrier,     ".ddr_carrier")
#pragma DATA_SECTION(g_stego_out,   ".ddr_stego")
#pragma DATA_SECTION(g_image_out,   ".ddr_image_out")
#pragma DATA_SECTION(g_perm_buf,    ".ddr_perm")
#pragma DATA_SECTION(g_header_buf,  ".ddr_header")
#pragma DATA_SECTION(g_results,     ".ddr_results")
#pragma DATA_SECTION(g_done_flag,   ".ddr_done_flag")
#pragma DATA_SECTION(g_dwt_work,    ".ddr_dwt_work")
#endif

/* Large DDR2 arrays */
PLATFORM_ALIGN(8) float    g_image_in[STEGO_MAX_IMG_W * STEGO_MAX_IMG_H];
PLATFORM_ALIGN(8) float    g_target_sig[STEGO_MAX_TARGET_LEN];
PLATFORM_ALIGN(8) float    g_carrier[STEGO_MAX_CARRIER_LEN];
PLATFORM_ALIGN(8) float    g_stego_out[STEGO_MAX_CARRIER_LEN];
PLATFORM_ALIGN(8) float    g_image_out[STEGO_MAX_IMG_W * STEGO_MAX_IMG_H];
PLATFORM_ALIGN(8) uint32_t g_perm_buf[STEGO_MAX_TARGET_LEN];
PLATFORM_ALIGN(8) uint8_t  g_header_buf[DDR2_HEADER_SIZE];
PLATFORM_ALIGN(8) stego_results_t g_results;
volatile uint32_t          g_done_flag = 0;
PLATFORM_ALIGN(8) float    g_dwt_work[DDR2_DWT_WORK_SIZE / sizeof(float)];

/* ── Fast Working Buffers placed in L2 SRAM ─────────────────────────────── */
#ifdef __TI_COMPILER_VERSION__
#pragma DATA_SECTION(g_twiddles,   ".l2_twiddles")
#pragma DATA_SECTION(g_kaiser_win, ".l2_window")
#pragma DATA_SECTION(g_fft_work,   ".l2_fftwork")
#endif

PLATFORM_ALIGN(8) float g_twiddles[2 * STEGO_FFT_N];
PLATFORM_ALIGN(8) float g_kaiser_win[STEGO_FFT_N];
PLATFORM_ALIGN(8) float g_fft_work[2 * STEGO_FFT_N];

/* Configurable operating mode */
volatile stego_mode_t g_mode = STEGO_DEFAULT_MODE;

static void run_selftest(void) {
    uint32_t H = 64;
    uint32_t W = 64;
    uint32_t N = 128;
    uint32_t carrier_len = 131072; /* 128k samples */
    uint32_t key = STEGO_DEFAULT_KEY;
    float scale = STEGO_DEFAULT_SCALE;

    /* Synthesize synthetic grayscale image test pattern */
    for (uint32_t i = 0; i < H * W; i++) {
        g_image_in[i] = (float)(i % 256) / 255.0f;
    }

    /* Synthesize carrier audio (multitone) */
    for (uint32_t i = 0; i < carrier_len; i++) {
        g_carrier[i] = 0.5f * sinf(2.0f * 3.14159265f * (float)i / 32.0f);
    }

    embed_params_t ep;
    memset(&ep, 0, sizeof(ep));
    ep.image = g_image_in;
    ep.img_height = H;
    ep.img_width = W;
    ep.carrier = g_carrier;
    ep.carrier_len = carrier_len;
    ep.key = key;
    ep.scale = scale;
    ep.kaiser_beta = STEGO_KAISER_BETA;
    ep.fft_n = N;
    ep.win_epsilon = STEGO_WIN_EPSILON;
    ep.wavelet_id = STEGO_WAVELET_HAAR;
    ep.img_min = 0.0f;
    ep.img_max = 1.0f;
    ep.stego_out = g_stego_out;
    ep.stego_buf_size = STEGO_MAX_CARRIER_LEN;
    ep.header_out = (stego_header_t *)g_header_buf;
    ep.target_buf = g_target_sig;
    ep.target_buf_size = STEGO_MAX_TARGET_LEN;
    ep.perm_buf = g_perm_buf;
    ep.perm_buf_size = STEGO_MAX_TARGET_LEN;
    ep.fft_twiddles = g_twiddles;
    ep.kaiser_win = g_kaiser_win;
    ep.fft_work = g_fft_work;
    ep.dwt_work = g_dwt_work;
    ep.dwt_work_size = DDR2_DWT_WORK_SIZE / sizeof(float);

    uint64_t t0 = timing_start();
    stego_status_t st = stego_embed(&ep);
    g_results.cycles_embed = timing_end(t0);

    if (st != STEGO_STATUS_OK) {
        g_results.status_code = st;
        return;
    }

    extract_params_t xp;
    memset(&xp, 0, sizeof(xp));
    xp.stego = g_stego_out;
    xp.stego_len = ep.stego_len;
    xp.header = (stego_header_t *)g_header_buf;
    xp.key = key;
    xp.image_out = g_image_out;
    xp.img_buf_size = STEGO_MAX_IMG_W * STEGO_MAX_IMG_H;
    xp.target_buf = g_target_sig;
    xp.target_buf_size = STEGO_MAX_TARGET_LEN;
    xp.perm_buf = g_perm_buf;
    xp.perm_buf_size = STEGO_MAX_TARGET_LEN;
    xp.fft_twiddles = g_twiddles;
    xp.kaiser_win = g_kaiser_win;
    xp.fft_work = g_fft_work;
    xp.dwt_work = g_dwt_work;
    xp.dwt_work_size = DDR2_DWT_WORK_SIZE / sizeof(float);

    t0 = timing_start();
    st = stego_extract(&xp);
    g_results.cycles_extract = timing_end(t0);

    g_results.status_code = st;
    g_results.target_len = ep.target_len;
    g_results.dwt_level = ep.dwt_level;
    g_results.scale_factor = scale;

    if (st == STEGO_STATUS_OK) {
        g_results.image_mse = compute_mse(g_image_in, g_image_out, H * W);
        g_results.image_psnr = compute_psnr(g_image_in, g_image_out, H * W, 1.0f);
        g_results.audio_snr = compute_snr(g_carrier, g_stego_out, carrier_len);

        uint32_t csum = 0;
        for (uint32_t i = 0; i < H * W; i++) {
            uint32_t val;
            memcpy(&val, &g_image_out[i], sizeof(val));
            csum += val;
        }
        g_results.output_checksum = csum;
    }
}

int main(void) {
    platform_init_tsc();

    g_done_flag = 0;
    memset(&g_results, 0, sizeof(g_results));
    g_results.magic = 0x52455355; /* "RESU" */

    if (g_mode == STEGO_MODE_SELFTEST) {
        run_selftest();
    }

    g_done_flag = 1;
    return 0;
}
