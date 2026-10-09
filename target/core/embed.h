#ifndef EMBED_H
#define EMBED_H
#include <stdint.h>
#include "config.h"
#include "io/header.h"

/* Full embedding pipeline.
 * See EXPLANATION.md Section 7 for data flow. */

typedef struct {
    /* Input parameters */
    const float *image;          /* normalized grayscale image [0,1] */
    uint32_t img_height;         /* H */
    uint32_t img_width;          /* W */
    const float *carrier;        /* carrier audio samples */
    uint32_t carrier_len;        /* number of carrier samples */
    uint32_t key;                /* encryption key */
    float scale;                 /* embedding scale factor */
    float kaiser_beta;           /* Kaiser window beta */
    uint32_t fft_n;              /* FFT size */
    float win_epsilon;           /* window division epsilon */
    uint32_t wavelet_id;         /* wavelet selection */
    float img_min, img_max;      /* original image range */
    
    /* Output pointers (caller provides buffers) */
    float *stego_out;            /* stego audio output */
    uint32_t stego_buf_size;     /* stego buffer capacity */
    stego_header_t *header_out;  /* filled header */
    
    /* Working buffers */
    float *target_buf;           /* target signal buffer */
    uint32_t target_buf_size;
    uint32_t *perm_buf;          /* permutation indices */
    uint32_t perm_buf_size;
    float *fft_twiddles;         /* precomputed twiddles */
    float *kaiser_win;           /* precomputed window */
    float *fft_work;             /* FFT working buffer */
    float *dwt_work;             /* DWT working buffer */
    uint32_t dwt_work_size;
    
    /* Output info */
    uint32_t target_len;         /* actual target signal length */
    uint32_t dwt_level;          /* computed DWT level */
    uint32_t stego_len;          /* actual stego audio length */
} embed_params_t;

stego_status_t stego_embed(embed_params_t *p);

#endif
