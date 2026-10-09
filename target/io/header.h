#ifndef HEADER_H
#define HEADER_H
#include <stdint.h>
#include "config.h"

/* Side-information header for stego embedding.
 * Stored in a sidecar .hdr file (not embedded in audio).
 * See EXPLANATION.md Section 6 for byte layout and tradeoffs.
 *
 * Design: sidecar file is simpler and avoids modifying the audio format.
 * Tradeoff: requires separate file management (documented). */

/* Packed, little-endian. Total = 64 bytes. */
typedef struct __attribute__((packed)) {
    uint32_t magic;            /* STEGO_HEADER_MAGIC = 0x53544547 */
    uint16_t version;          /* STEGO_HEADER_VERSION */
    uint16_t reserved;         /* padding */
    uint16_t img_width;        /* image width in pixels */
    uint16_t img_height;       /* image height in pixels */
    uint16_t fft_n;            /* FFT size (e.g. 1024) */
    uint16_t wavelet_id;       /* STEGO_WAVELET_HAAR or _DB2 */
    float    kaiser_beta;      /* Kaiser window beta */
    float    scale_factor;     /* embedding scale */
    float    img_min;          /* original image min (for denormalization) */
    float    img_max;          /* original image max (for denormalization) */
    uint32_t dwt_level;        /* number of DWT cascade levels */
    uint32_t original_carrier_len;  /* carrier length before padding */
    uint32_t target_len;       /* target signal length */
    uint32_t key_check;        /* hash of the key (NEVER the key itself) */
    uint8_t  pad[12];          /* pad to exactly 64 bytes */
    uint32_t crc32;            /* CRC-32 of all preceding fields */
} stego_header_t;

/* Compile-time size check */
STEGO_STATIC_ASSERT(sizeof(stego_header_t) == 64, header_size_must_be_64);

/* Write header to buffer (computes CRC automatically).
 * buf must be at least sizeof(stego_header_t) bytes. */
stego_status_t header_write(stego_header_t *hdr, void *buf, uint32_t buf_size);

/* Read and validate header from buffer.
 * Checks magic, version, and CRC. */
stego_status_t header_read(const void *buf, uint32_t buf_size, stego_header_t *hdr);

#endif
