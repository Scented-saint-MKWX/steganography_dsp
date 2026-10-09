#include "io/header.h"
#include "io/crc32.h"
#include <string.h>

stego_status_t header_write(stego_header_t *hdr, void *buf, uint32_t buf_size) {
    if (!hdr || !buf) return STEGO_ERR_NULL_PTR;
    if (buf_size < sizeof(stego_header_t)) return STEGO_ERR_INVALID_PARAM;
    
    hdr->magic = 0x53544547; /* 'STEG' */
    hdr->version = 1;
    
    // Calculate CRC of everything except the crc32 field itself
    hdr->crc32 = crc32_compute(hdr, sizeof(stego_header_t) - sizeof(uint32_t));
    
    memcpy(buf, hdr, sizeof(stego_header_t));
    return STEGO_STATUS_OK;
}

stego_status_t header_read(const void *buf, uint32_t buf_size, stego_header_t *hdr) {
    if (!buf || !hdr) return STEGO_ERR_NULL_PTR;
    if (buf_size < sizeof(stego_header_t)) return STEGO_ERR_INVALID_PARAM;
    
    memcpy(hdr, buf, sizeof(stego_header_t));
    
    if (hdr->magic != 0x53544547) return STEGO_ERR_INVALID_HEADER;
    
    uint32_t expected_crc = crc32_compute(hdr, sizeof(stego_header_t) - sizeof(uint32_t));
    if (hdr->crc32 != expected_crc) return STEGO_ERR_CRC_MISMATCH;
    
    return STEGO_STATUS_OK;
}
