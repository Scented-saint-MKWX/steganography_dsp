#ifndef EXTRACT_H
#define EXTRACT_H
#include <stdint.h>
#include "../config.h"
#include "../io/header.h"

/*
 * Full extraction pipeline.
 * See EXPLANATION.md Section 7 for data flow.
 */

typedef struct {
    /* Inputs */
    const float *stego;          /* stego audio samples */
    uint32_t stego_len;          /* number of stego samples */
    const stego_header_t *header; /* side-info header */
    uint32_t key;                /* user-provided key */

    /* Output */
    float *image_out;            /* reconstructed image (row-major, float) */
    uint32_t img_buf_size;       /* image buffer capacity in floats */

    /* Working buffers (caller provides) */
    float *target_buf;           /* target signal scratch */
    uint32_t target_buf_size;    /* capacity in floats */
    uint32_t *perm_buf;          /* permutation indices */
    uint32_t perm_buf_size;      /* capacity in uint32_t */
    float *fft_twiddles;         /* precomputed twiddles, 2*N floats */
    float *kaiser_win;           /* precomputed window, N floats */
    float *fft_work;             /* FFT working buffer, 2*N floats */
    float *dwt_work;             /* DWT working buffer */
    uint32_t dwt_work_size;      /* DWT work capacity in floats */
} extract_params_t;

stego_status_t stego_extract(extract_params_t *p);

#endif /* EXTRACT_H */
