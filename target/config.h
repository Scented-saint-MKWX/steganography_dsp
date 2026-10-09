/*
 * config.h - Central configuration for DSP Steganography project
 *
 * All tunables live here. Both host and target builds include this.
 * See EXPLANATION.md Section 3 for the rationale behind each parameter.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ─── Version ──────────────────────────────────────────────────────── */
#define STEGO_VERSION_MAJOR  1
#define STEGO_VERSION_MINOR  0

/* ─── STFT / Window parameters ─────────────────────────────────────── */

/* FFT length per frame (must be power of 2).
 * Valid: 16, 32, 64, 128, 256, 512, 1024.
 * Paper default: 1024 -> 512 useful frequency bins per column.
 * Changing N changes the number of frequency bins per image column;
 * with N=1024, a 512-row image maps to 512 bins + 512 mirrored bins.
 * See EXPLANATION.md Section 3.1 and Table 2 sweep. */
#define STEGO_FFT_N          1024

/* Kaiser window beta. Paper default: 1.0.
 * WARNING: beta >= ~15 destroys the image (paper Table 1).
 * beta = 0 gives rectangular window, beta = 1 gives mild tapering. */
#define STEGO_KAISER_BETA    1.0f

/* Overlap length (paper specifies 0, do not change without
 * updating the overlap-add logic in stft_img.c) */
#define STEGO_OVERLAP        0

/* Epsilon for window division (guard against division by near-zero).
 * If |w[n]| < epsilon, clamp to epsilon. */
#define STEGO_WIN_EPSILON    1e-7f

/* ─── Image parameters ──────────────────────────────────────────────── */

/* Maximum supported image dimensions */
#define STEGO_MAX_IMG_W      512
#define STEGO_MAX_IMG_H      512

/* Number of useful frequency bins per column = FFT_N / 2 */
#define STEGO_FREQ_BINS      (STEGO_FFT_N / 2)

/* ─── Target Signal parameters ──────────────────────────────────────── */

/* Maximum target signal length = MAX_IMG_W * FFT_N
 * For 512x512 image with N=1024: 512 * 1024 = 524288 */
#define STEGO_MAX_TARGET_LEN (STEGO_MAX_IMG_W * STEGO_FFT_N)

/* ─── DWT parameters ────────────────────────────────────────────────── */

/* Wavelet selection (compile-time).
 * 0 = Haar, 1 = db2 (Daubechies-2).
 * Paper does not name a wavelet; Haar is default. */
#define STEGO_WAVELET_ID     0
#define STEGO_WAVELET_HAAR   0
#define STEGO_WAVELET_DB2    1

/* Maximum DWT levels to consider */
#define STEGO_MAX_DWT_LEVELS 16

/* ─── Carrier audio parameters ──────────────────────────────────────── */

/* Maximum carrier length in float samples.
 * 4.2 million samples ≈ 95 seconds at 44100 Hz.
 * 4200000 * 4 bytes = 16.8 MB in DDR2. */
#define STEGO_MAX_CARRIER_LEN  4200000

/* ─── Scaling ────────────────────────────────────────────────────────── */

/* Default scale factor for embedding (makes hidden signal inaudible).
 * Lower = more inaudible but more susceptible to quantization noise. */
#define STEGO_DEFAULT_SCALE  0.01f

/* ─── Encryption / PRNG ──────────────────────────────────────────────── */

/* Default key for permutation (any nonzero 32-bit integer) */
#define STEGO_DEFAULT_KEY    0xDEADBEEFu

/* Key check: we store a hash of the key (never the key itself).
 * Uses a simple mixing function, not cryptographic.
 * See EXPLANATION.md Section 5 for security discussion. */

/* ─── Side-info header ───────────────────────────────────────────────── */

/* Magic number for header validation */
#define STEGO_HEADER_MAGIC   0x53544547u  /* "STEG" in little-endian */

/* Header version */
#define STEGO_HEADER_VERSION 1

/* ─── DDR2 Memory Map ────────────────────────────────────────────────── */
/* Base: 0xC0000000, Size: 128 MB = 0x08000000
 * End:  0xC7FFFFFF
 *
 * Layout (non-overlapping, all 8-byte aligned):
 *   0xC0000000 .. 0xC01FFFFF  Input image buffer     (2 MB, 512*512*4=1MB used)
 *   0xC0200000 .. 0xC03FFFFF  Target signal buffer   (2 MB, 524288*4=2MB)
 *   0xC0400000 .. 0xC13FFFFF  Carrier audio input    (16 MB, 4.2M*4≈16.8MB)
 *   0xC1400000 .. 0xC23FFFFF  Stego audio output     (16 MB)
 *   0xC2400000 .. 0xC25FFFFF  Extracted image buffer (2 MB)
 *   0xC2600000 .. 0xC27FFFFF  Permutation index buf  (2 MB, 524288*4=2MB)
 *   0xC2800000 .. 0xC28000FF  Side-info header       (256 bytes)
 *   0xC2800100 .. 0xC28001FF  Results struct         (256 bytes)
 *   0xC2800200 .. 0xC2800203  Done flag              (4 bytes)
 *   0xC2800204 .. 0xC3FFFFFF  DWT working buffer     (24 MB)
 *   --- free space up to 0xC7FFFFFF ---
 *
 * Total used: ~63 MB of 128 MB.
 * See docs/memory_map.md for the generated version.
 */

#define DDR2_BASE             0xC0000000u
#define DDR2_SIZE             0x08000000u  /* 128 MB */

#define DDR2_IMAGE_IN_ADDR    0xC0000000u
#define DDR2_IMAGE_IN_SIZE    0x00200000u  /* 2 MB */

#define DDR2_TARGET_ADDR      0xC0200000u
#define DDR2_TARGET_SIZE      0x00200000u  /* 2 MB */

#define DDR2_CARRIER_ADDR     0xC0400000u
#define DDR2_CARRIER_SIZE     0x01000000u  /* 16 MB */

#define DDR2_STEGO_ADDR       0xC1400000u
#define DDR2_STEGO_SIZE       0x01000000u  /* 16 MB */

#define DDR2_IMAGE_OUT_ADDR   0xC2400000u
#define DDR2_IMAGE_OUT_SIZE   0x00200000u  /* 2 MB */

#define DDR2_PERM_ADDR        0xC2600000u
#define DDR2_PERM_SIZE        0x00200000u  /* 2 MB */

#define DDR2_HEADER_ADDR      0xC2800000u
#define DDR2_HEADER_SIZE      0x00000100u  /* 256 bytes */

#define DDR2_RESULTS_ADDR     0xC2800100u
#define DDR2_RESULTS_SIZE     0x00000100u  /* 256 bytes */

#define DDR2_DONE_FLAG_ADDR   0xC2800200u
#define DDR2_DONE_FLAG_SIZE   0x00000004u  /* 4 bytes */

#define DDR2_DWT_WORK_ADDR    0xC2800204u
#define DDR2_DWT_WORK_SIZE    0x01800000u  /* 24 MB */

/* ─── Compile-time region overlap check ──────────────────────────────── */
/* Each region must end before the next begins, and all must fit in DDR2. */

#define STEGO_REGION_END(addr, size) ((addr) + (size))

#define STEGO_STATIC_ASSERT(cond, msg) \
    typedef char static_assert_##msg[(cond) ? 1 : -1]

STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_IMAGE_IN_ADDR, DDR2_IMAGE_IN_SIZE) <= DDR2_TARGET_ADDR,
                     image_in_overlaps_target);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_TARGET_ADDR, DDR2_TARGET_SIZE) <= DDR2_CARRIER_ADDR,
                     target_overlaps_carrier);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_CARRIER_ADDR, DDR2_CARRIER_SIZE) <= DDR2_STEGO_ADDR,
                     carrier_overlaps_stego);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_STEGO_ADDR, DDR2_STEGO_SIZE) <= DDR2_IMAGE_OUT_ADDR,
                     stego_overlaps_image_out);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_IMAGE_OUT_ADDR, DDR2_IMAGE_OUT_SIZE) <= DDR2_PERM_ADDR,
                     image_out_overlaps_perm);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_PERM_ADDR, DDR2_PERM_SIZE) <= DDR2_HEADER_ADDR,
                     perm_overlaps_header);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_HEADER_ADDR, DDR2_HEADER_SIZE) <= DDR2_RESULTS_ADDR,
                     header_overlaps_results);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_RESULTS_ADDR, DDR2_RESULTS_SIZE) <= DDR2_DONE_FLAG_ADDR,
                     results_overlaps_done_flag);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_DONE_FLAG_ADDR, DDR2_DONE_FLAG_SIZE) <= DDR2_DWT_WORK_ADDR,
                     done_flag_overlaps_dwt_work);
STEGO_STATIC_ASSERT(STEGO_REGION_END(DDR2_DWT_WORK_ADDR, DDR2_DWT_WORK_SIZE) <= DDR2_BASE + DDR2_SIZE,
                     dwt_work_exceeds_ddr2);

/* ─── L2 SRAM budget (C6748 has 256 KB L2) ────────────────────────── */
/*   Code + stack + heap:        ~64 KB (estimate)
 *   FFT twiddle factors:         8 KB  (1024 complex floats)
 *   Kaiser window:               4 KB  (1024 floats)
 *   Per-frame working buffer:    8 KB  (1024 complex floats)
 *   DWT filter coefficients:     <1 KB
 *   ---
 *   Total ~85 KB, well within 256 KB.
 *   See EXPLANATION.md Section 9 for details.
 */
#define L2_SRAM_SIZE          (256u * 1024u)

/* ─── Results / profiling ────────────────────────────────────────────── */

/* Status codes for the results struct */
typedef enum {
    STEGO_STATUS_OK              = 0,
    STEGO_STATUS_ERR_BAD_ARGS    = 1,
    STEGO_STATUS_ERR_CARRIER_SHORT = 2,
    STEGO_STATUS_ERR_BAD_HEADER  = 3,
    STEGO_STATUS_ERR_BAD_CRC     = 4,
    STEGO_STATUS_ERR_BAD_MAGIC   = 5,
    STEGO_STATUS_ERR_BAD_VERSION = 6,
    STEGO_STATUS_ERR_BAD_KEY     = 7,
    STEGO_STATUS_ERR_IMG_TOO_BIG = 8,
    STEGO_STATUS_ERR_FFT_SIZE    = 9,
    STEGO_STATUS_ERR_DWT_LEVEL   = 10,
    STEGO_STATUS_ERR_INTERNAL    = 99,
    STEGO_STATUS_ERROR_INVALID_ARG = 100
} stego_status_t;

/* Status code aliases for compatibility */
#define STEGO_ERR_NULL_PTR         STEGO_STATUS_ERR_BAD_ARGS
#define STEGO_ERR_INVALID_PARAM    STEGO_STATUS_ERR_BAD_ARGS
#define STEGO_ERR_BUFFER_TOO_SMALL STEGO_STATUS_ERR_BAD_ARGS
#define STEGO_ERR_CAPACITY         STEGO_STATUS_ERR_CARRIER_SHORT
#define STEGO_ERR_INVALID_HEADER   STEGO_STATUS_ERR_BAD_HEADER
#define STEGO_ERR_CRC_MISMATCH     STEGO_STATUS_ERR_BAD_CRC
#define STEGO_ERR_KEY_MISMATCH     STEGO_STATUS_ERR_BAD_KEY
#define STEGO_ERR_FILE_IO          STEGO_STATUS_ERR_INTERNAL

/* ─── Operating modes ────────────────────────────────────────────────── */
typedef enum {
    STEGO_MODE_EMBED    = 0,
    STEGO_MODE_EXTRACT  = 1,
    STEGO_MODE_SELFTEST = 2
} stego_mode_t;

/* The mode is set by the host before running, or defaulted in main.c. */
#define STEGO_DEFAULT_MODE   STEGO_MODE_SELFTEST

#endif /* CONFIG_H */
