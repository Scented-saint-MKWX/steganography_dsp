#include "io/dataio.h"

#if PLATFORM_HOST
#include <stdio.h>

stego_status_t read_float32_bin(const char *path, float *buf, uint32_t max_count, uint32_t *actual_count) {
    if (!path || !buf || !actual_count) return STEGO_ERR_NULL_PTR;
    FILE *f = fopen(path, "rb");
    if (!f) return STEGO_ERR_FILE_IO;
    size_t read_cnt = fread(buf, sizeof(float), max_count, f);
    *actual_count = (uint32_t)read_cnt;
    fclose(f);
    return STEGO_STATUS_OK;
}

stego_status_t write_float32_bin(const char *path, const float *buf, uint32_t count) {
    if (!path || !buf) return STEGO_ERR_NULL_PTR;
    FILE *f = fopen(path, "wb");
    if (!f) return STEGO_ERR_FILE_IO;
    size_t written = fwrite(buf, sizeof(float), count, f);
    fclose(f);
    return (written == count) ? STEGO_STATUS_OK : STEGO_ERR_FILE_IO;
}

stego_status_t read_header_file(const char *path, stego_header_t *hdr) {
    if (!path || !hdr) return STEGO_ERR_NULL_PTR;
    FILE *f = fopen(path, "rb");
    if (!f) return STEGO_ERR_FILE_IO;
    uint8_t buf[64];
    if (fread(buf, 1, 64, f) != 64) {
        fclose(f);
        return STEGO_ERR_FILE_IO;
    }
    fclose(f);
    return header_read(buf, 64, hdr);
}

stego_status_t write_header_file(const char *path, stego_header_t *hdr) {
    if (!path || !hdr) return STEGO_ERR_NULL_PTR;
    FILE *f = fopen(path, "wb");
    if (!f) return STEGO_ERR_FILE_IO;
    uint8_t buf[64];
    stego_status_t st = header_write(hdr, buf, 64);
    if (st != STEGO_STATUS_OK) {
        fclose(f);
        return st;
    }
    if (fwrite(buf, 1, 64, f) != 64) {
        fclose(f);
        return STEGO_ERR_FILE_IO;
    }
    fclose(f);
    return STEGO_STATUS_OK;
}

#endif
